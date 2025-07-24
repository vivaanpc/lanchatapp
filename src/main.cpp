#include "message/message_handler.hpp"
#include "network/peer_discovery.hpp"
#include "http/http_server.hpp"
#include <iostream>

int main() {
    try {
        MessageHandler msgHandler("messages.json");
        PeerDiscovery peerDiscovery;
        HttpServer server(8080, msgHandler, peerDiscovery);

        peerDiscovery.start();
        server.start();

        std::cout << "LAN Chat app running on http://localhost:8080. Press Enter to stop...\n";
        std::cin.get();

        server.stop();
        peerDiscovery.stop();
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}