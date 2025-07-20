#pragma once

#include <string>
#include <thread>
#include <atomic>
#include <memory>

// Forward declarations
class MessageHandler;
class PeerDiscovery;

struct HttpRequest {
    std::string method;
    std::string path;
    std::string query;
    std::string body;
    std::unordered_map<std::string, std::string> headers;
};

struct HttpResponse {
    int statusCode;
    std::string statusText;
    std::string body;
    std::unordered_map<std::string, std::string> headers;
    
    HttpResponse(int code = 200, const std::string& text = "OK") 
        : statusCode(code), statusText(text) {}
};

class HttpServer {
private:
    int port;
    std::atomic<bool> running;
    std::thread serverThread;
    
    MessageHandler& messageHandler;
    PeerDiscovery& peerDiscovery;
    
    void serverLoop();
    void handleClient(int clientSocket);
    
    bool parseRequest(const std::string& requestData, HttpRequest& request);
    std::string buildResponse(const HttpResponse& response);
    
    HttpResponse handleGetRoot();
    HttpResponse handleGetAsset(const std::string& path);
    HttpResponse handleGetMessages();
    HttpResponse handlePostMessages(const std::string& body);
    HttpResponse handleGetPeers();
    
    std::string getContentType(const std::string& path);
    std::string loadWebFile(const std::string& filename);
    std::string urlDecode(const std::string& str);
    
public:
    HttpServer(int serverPort, MessageHandler& msgHandler, PeerDiscovery& peerDisc);
    ~HttpServer();
    
    void start();
    void stop();
    
    bool isRunning() const { return running; }
    
    // Disable copy constructor and assignment
    HttpServer(const HttpServer&) = delete;
    HttpServer& operator=(const HttpServer&) = delete;
};