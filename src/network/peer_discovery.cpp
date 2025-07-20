#include "peer_discovery.hpp"
#include "sockets.hpp"
#include "json.hpp"
#include <iostream>
#include <chrono>
#include <thread>
#include <sstream>
#include <random>

PeerDiscovery::PeerDiscovery() : running(false), discoveryPort(9999) {
    // Generate a unique peer ID
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(10000, 99999);
    peerId = "peer_" + std::to_string(dis(gen));
}

PeerDiscovery::~PeerDiscovery() {
    stop();
}

void PeerDiscovery::start() {
    if (running) return;
    
    running = true;
    std::cout << "Starting peer discovery with ID: " << peerId << std::endl;
    
    // Start broadcast thread
    broadcastThread = std::thread(&PeerDiscovery::broadcastLoop, this);
    
    // Start listener thread
    listenerThread = std::thread(&PeerDiscovery::listenerLoop, this);
}

void PeerDiscovery::stop() {
    if (!running) return;
    
    running = false;
    
    if (broadcastThread.joinable()) {
        broadcastThread.join();
    }
    
    if (listenerThread.joinable()) {
        listenerThread.join();
    }
    
    std::cout << "Peer discovery stopped." << std::endl;
}

std::vector<PeerInfo> PeerDiscovery::getActivePeers() {
    std::lock_guard<std::mutex> lock(peersMutex);
    std::vector<PeerInfo> activePeers;
    
    auto now = std::chrono::steady_clock::now();
    
    for (const auto& pair : peers) {
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - pair.second.lastSeen);
        if (elapsed.count() < 90) { // 90 second timeout
            activePeers.push_back(pair.second);
        }
    }
    
    return activePeers;
}

void PeerDiscovery::broadcastLoop() {
    UDPSocket broadcastSocket;
    
    if (!broadcastSocket.setBroadcast(true)) {
        std::cerr << "Failed to enable broadcast on socket" << std::endl;
        return;
    }
    
    while (running) {
        // Create discovery message
        nlohmann::json message;
        message["type"] = "discovery";
        message["service"] = "lanchat";
        message["peer_id"] = peerId;
        message["timestamp"] = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        
        std::string jsonString = message.dump();
        
        // Broadcast to subnet
        if (!broadcastSocket.sendTo(jsonString, "255.255.255.255", discoveryPort)) {
            std::cerr << "Failed to send broadcast message" << std::endl;
        }
        
        // Wait 30 seconds before next broadcast
        for (int i = 0; i < 30 && running; ++i) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
}

void PeerDiscovery::listenerLoop() {
    UDPSocket listenerSocket;
    
    if (!listenerSocket.bind(discoveryPort)) {
        std::cerr << "Failed to bind listener socket to port " << discoveryPort << std::endl;
        return;
    }
    
    std::cout << "Listening for peer discovery messages on port " << discoveryPort << std::endl;
    
    while (running) {
        std::string data;
        std::string fromAddress;
        int fromPort;
        
        if (listenerSocket.receiveFrom(data, fromAddress, fromPort, 1000)) {
            try {
                nlohmann::json message = nlohmann::json::parse(data);
                
                // Validate message format
                if (message.contains("type") && message["type"] == "discovery" &&
                    message.contains("service") && message["service"] == "lanchat" &&
                    message.contains("peer_id")) {
                    
                    std::string receivedPeerId = message["peer_id"];
                    
                    // Don't add ourselves
                    if (receivedPeerId != peerId) {
                        std::lock_guard<std::mutex> lock(peersMutex);
                        
                        PeerInfo& peer = peers[receivedPeerId];
                        peer.id = receivedPeerId;
                        peer.address = fromAddress;
                        peer.lastSeen = std::chrono::steady_clock::now();
                        
                        // If this is a new peer, log it
                        bool isNewPeer = peer.address != fromAddress || 
                                        std::chrono::duration_cast<std::chrono::seconds>(
                                            std::chrono::steady_clock::now() - peer.lastSeen).count() > 120;
                        
                        if (isNewPeer) {
                            std::cout << "Discovered peer: " << receivedPeerId 
                                     << " at " << fromAddress << std::endl;
                        }
                    }
                }
            } catch (const std::exception& e) {
                // Ignore invalid JSON messages
                std::cerr << "Invalid discovery message received: " << e.what() << std::endl;
            }
        }
        
        // Clean up expired peers
        cleanupExpiredPeers();
    }
}

void PeerDiscovery::cleanupExpiredPeers() {
    std::lock_guard<std::mutex> lock(peersMutex);
    
    auto now = std::chrono::steady_clock::now();
    auto it = peers.begin();
    
    while (it != peers.end()) {
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - it->second.lastSeen);
        if (elapsed.count() >= 90) { // 90 second timeout
            std::cout << "Peer " << it->first << " has timed out." << std::endl;
            it = peers.erase(it);
        } else {
            ++it;
        }
    }
}