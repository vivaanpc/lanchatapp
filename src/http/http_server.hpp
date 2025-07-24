#pragma once

#include <string>
#include <unordered_map>
#include <thread>
#include <atomic>
#include "../message/message_handler.hpp"
#include "../network/peer_discovery.hpp"
#include "../network/sockets.hpp"
#include "../util/utils.hpp"

struct HttpRequest {
    std::string method;
    std::string path;
    std::string body;
    std::unordered_map<std::string, std::string> headers;
};

class HttpServer {
public:
    HttpServer(int port, MessageHandler& msgHandler, PeerDiscovery& peerDisc);
    ~HttpServer();
    void start();
    void stop();
private:
    void serverLoop();
    void handleClient(SOCKET clientSock);
    HttpRequest parseRequest(const std::string& raw);
    std::string buildResponse(const std::string& body, const std::string& contentType = "application/json", int code = 200);
    std::string handleGet(const HttpRequest& req);
    std::string handlePost(const HttpRequest& req);

    int port;
    MessageHandler& msgHandler;
    PeerDiscovery& peerDisc;
    std::atomic<bool> running{false};
    std::thread serverTh;
    TCPServer tcpServer;
};
