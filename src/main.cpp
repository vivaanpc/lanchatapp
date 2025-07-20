#include <iostream>
#include <thread>
#include <chrono>
#include <signal.h>
#include <atomic>
#include "http/http_server.hpp"
#include "network/peer_discovery.hpp"
#include "message/message_handler.hpp"

std::atomic<bool> running(true);

void signalHandler(int signum) {
    std::cout << "\nReceived signal " << signum << ". Shutting down gracefully..." << std::endl;
    running = false;
}

int main(int argc, char* argv[]) {
    // Set up signal handlers
    signal(SIGINT, signalHandler);
    #ifndef WIN32
    signal(SIGTERM, signalHandler);
    #endif

    // Parse command line arguments
    int port = 8080;
    if (argc > 2 && std::string(argv[1]) == "--port") {
        port = std::stoi(argv[2]);
    }

    std::cout << "Starting LAN Chat Application..." << std::endl;
    std::cout << "HTTP Server will run on http://localhost:" << port << std::endl;

    try {
        // Initialize components
        MessageHandler messageHandler("data/messages.json");
        PeerDiscovery peerDiscovery;
        HttpServer httpServer(port, messageHandler, peerDiscovery);

        // Start peer discovery
        std::thread peerThread([&peerDiscovery]() {
            peerDiscovery.start();
        });

        // Start HTTP server
        std::thread httpThread([&httpServer]() {
            httpServer.start();
        });

        std::cout << "Application started successfully!" << std::endl;
        std::cout << "Open your browser and go to http://localhost:" << port << std::endl;
        std::cout << "Press Ctrl+C to stop." << std::endl;

        // Main loop
        while (running) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        // Shutdown
        std::cout << "Shutting down components..." << std::endl;
        httpServer.stop();
        peerDiscovery.stop();

        if (peerThread.joinable()) {
            peerThread.join();
        }
        if (httpThread.joinable()) {
            httpThread.join();
        }

        std::cout << "Application stopped successfully." << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}