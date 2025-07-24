#include "peer_discovery.hpp"
#include "../util/utils.hpp"
#include <random>
#include <iostream>

PeerDiscovery::PeerDiscovery() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(1000, 9999);
    peerId = "peer_" + std::to_string(dis(gen));
    SocketUtils::initialize();
}

PeerDiscovery::~PeerDiscovery() {
    stop();
    SocketUtils::cleanup();
}

void PeerDiscovery::start() {
    if (running) return;
    running = true;
    broadcastTh = std::thread(&PeerDiscovery::broadcastLoop, this);
    listenTh = std::thread(&PeerDiscovery::listenLoop, this);
}

void PeerDiscovery::stop() {
    if (!running) return;
    running = false;
    if (broadcastTh.joinable()) broadcastTh.join();
    if (listenTh.joinable()) listenTh.join();
}

std::vector<PeerInfo> PeerDiscovery::getActivePeers() {
    std::lock_guard<std::mutex> lock(peersMutex);
    cleanupExpired();
    std::vector<PeerInfo> active;
    for (const auto& [_, info] : peers) {
        active.push_back(info);
    }
    return active;
}

void PeerDiscovery::broadcastLoop() {
    UDPSocket sock;
    if (!sock.setBroadcast(true) || !sock.bind(0)) {
        std::cerr << "Broadcast setup failed\n";
        running = false;
        return;
    }
    while (running) {
        std::string msg = "DISCOVER:" + peerId + ":" + Utils::getCurrentTimeString();
        sock.sendTo(msg, "255.255.255.255", port);
        std::this_thread::sleep_for(std::chrono::seconds(5));
    }
}

void PeerDiscovery::listenLoop() {
    UDPSocket sock;
    if (!sock.bind(port)) {
        std::cerr << "Listener bind failed\n";
        running = false;
        return;
    }
    while (running) {
        std::string data, ip;
        int fromPort;
        if (sock.receiveFrom(data, ip, fromPort)) {
            auto parts = Utils::split(data, ':');
            if (parts.size() >= 2 && parts[0] == "DISCOVER") {
                std::string id = parts[1];
                if (id != peerId) {
                    std::lock_guard<std::mutex> lock(peersMutex);
                    peers[id].id = id;
                    peers[id].address = ip;
                    peers[id].lastSeen = std::chrono::steady_clock::now();
                }
            }
        }
    }
}

void PeerDiscovery::cleanupExpired() {
    auto now = std::chrono::steady_clock::now();
    for (auto it = peers.begin(); it != peers.end(); ) {
        if (std::chrono::duration_cast<std::chrono::seconds>(now - it->second.lastSeen).count() > 30) {
            it = peers.erase(it);
        } else {
            ++it;
        }
    }
}
