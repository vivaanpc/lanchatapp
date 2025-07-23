#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <thread>
#include <atomic>
#include <chrono>
#include "sockets.hpp"

struct PeerInfo {
    std::string id;
    std::string address;
    std::chrono::steady_clock::time_point lastSeen;
};

class PeerDiscovery {
public:
    PeerDiscovery();
    ~PeerDiscovery();
    void start();
    void stop();
    std::vector<PeerInfo> getActivePeers();
private:
    void broadcastLoop();
    void listenLoop();
    void cleanupExpired();

    std::string peerId;
    int port = 45454;
    std::atomic<bool> running{false};
    std::thread broadcastTh;
    std::thread listenTh;
    std::unordered_map<std::string, PeerInfo> peers;
    std::mutex peersMutex;
};
