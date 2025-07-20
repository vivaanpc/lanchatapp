#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <thread>
#include <mutex>
#include <chrono>
#include <atomic>

struct PeerInfo {
    std::string id;
    std::string address;
    std::chrono::steady_clock::time_point lastSeen;
    
    PeerInfo() = default;
    PeerInfo(const std::string& peerId, const std::string& peerAddress)
        : id(peerId), address(peerAddress), lastSeen(std::chrono::steady_clock::now()) {}
};

class PeerDiscovery {
private:
    std::atomic<bool> running;
    std::string peerId;
    int discoveryPort;
    
    std::thread broadcastThread;
    std::thread listenerThread;
    
    std::unordered_map<std::string, PeerInfo> peers;
    std::mutex peersMutex;
    
    void broadcastLoop();
    void listenerLoop();
    void cleanupExpiredPeers();
    
public:
    PeerDiscovery();
    ~PeerDiscovery();
    
    void start();
    void stop();
    
    std::vector<PeerInfo> getActivePeers();
    std::string getPeerId() const { return peerId; }
    
    // Disable copy constructor and assignment
    PeerDiscovery(const PeerDiscovery&) = delete;
    PeerDiscovery& operator=(const PeerDiscovery&) = delete;
};