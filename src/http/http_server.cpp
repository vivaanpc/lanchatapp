#include "http_server.hpp"
#include "message/message_handler.hpp"
#include "network/peer_discovery.hpp"
#include "network/sockets.hpp"
#include "json.hpp"
#include <iostream>
#include <sstream>
#include <fstream>
#include <unordered_map>
#include <algorithm>

HttpServer::HttpServer(int serverPort, MessageHandler& msgHandler, PeerDiscovery& peerDisc)
    : port(serverPort), running(false), messageHandler(msgHandler), peerDiscovery(peerDisc) {
}

HttpServer::~HttpServer() {
    stop();
}

void HttpServer::start() {
    if (running) return;
    
    running = true;
    serverThread = std::thread(&HttpServer::serverLoop, this);
    std::cout << "HTTP server started on port " << port << std::endl;
}

void HttpServer::stop() {
    if (!running) return;
    
    running = false;
    
    if (serverThread.joinable()) {
        serverThread.join();
    }
    
    std::cout << "HTTP server stopped." << std::endl;
}

void HttpServer::serverLoop() {
    NetworkUtils::initialize();
    
    // Create listening socket
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0) {
        std::cerr << "Failed to create server socket" << std::endl;
        return;
    }
    
    // Set socket options
    int opt = 1;
    if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt)) < 0) {
        std::cerr << "setsockopt failed" << std::endl;
        closesocket(serverSocket);
        return;
    }
    
    // Bind socket
    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);
    
    if (bind(serverSocket, (struct sockaddr*)&address, sizeof(address)) < 0) {
        std::cerr << "Bind failed on port " << port << std::endl;
        closesocket(serverSocket);
        return;
    }
    
    // Listen for connections
    if (listen(serverSocket, 10) < 0) {
        std::cerr << "Listen failed" << std::endl;
        closesocket(serverSocket);
        return;
    }
    
    std::cout << "HTTP server listening on port " << port << std::endl;
    
    // Accept connections
    while (running) {
        struct sockaddr_in clientAddr;
        socklen_t clientLen = sizeof(clientAddr);
        
        // Set socket to non-blocking for accept timeout
        NetworkUtils::setNonBlocking(serverSocket, true);
        
        int clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddr, &clientLen);
        
        if (clientSocket >= 0) {
            // Handle client in separate thread
            std::thread clientThread(&HttpServer::handleClient, this, clientSocket);
            clientThread.detach();
        } else {
            // Brief sleep to prevent busy waiting
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
    
    closesocket(serverSocket);
    NetworkUtils::cleanup();
}

void HttpServer::handleClient(int clientSocket) {
    char buffer[4096] = {0};
    int bytesRead = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
    
    if (bytesRead > 0) {
        std::string requestData(buffer, bytesRead);
        
        HttpRequest request;
        if (parseRequest(requestData, request)) {
            HttpResponse response;
            
            // Route the request
            if (request.method == "GET") {
                if (request.path == "/" || request.path == "/index.html") {
                    response = handleGetRoot();
                } else if (request.path.find("/api/messages") == 0) {
                    response = handleGetMessages();
                } else if (request.path.find("/api/peers") == 0) {
                    response = handleGetPeers();
                } else {
                    response = handleGetAsset(request.path);
                }
            } else if (request.method == "POST") {
                if (request.path == "/api/messages") {
                    response = handlePostMessages(request.body);
                } else {
                    response = HttpResponse(404, "Not Found");
                    response.body = "404 - Not Found";
                }
            } else {
                response = HttpResponse(405, "Method Not Allowed");
                response.body = "405 - Method Not Allowed";
            }
            
            // Send response
            std::string responseStr = buildResponse(response);
            send(clientSocket, responseStr.c_str(), responseStr.length(), 0);
        }
    }
    
    closesocket(clientSocket);
}

bool HttpServer::parseRequest(const std::string& requestData, HttpRequest& request) {
    std::istringstream iss(requestData);
    std::string line;
    
    // Parse request line
    if (!std::getline(iss, line)) return false;
    
    std::istringstream requestLine(line);
    std::string pathAndQuery;
    if (!(requestLine >> request.method >> pathAndQuery)) return false;
    
    // Parse path and query
    size_t queryPos = pathAndQuery.find('?');
    if (queryPos != std::string::npos) {
        request.path = pathAndQuery.substr(0, queryPos);
        request.query = pathAndQuery.substr(queryPos + 1);
    } else {
        request.path = pathAndQuery;
    }
    
    // Parse headers
    while (std::getline(iss, line) && line != "\r" && !line.empty()) {
        size_t colonPos = line.find(':');
        if (colonPos != std::string::npos) {
            std::string key = line.substr(0, colonPos);
            std::string value = line.substr(colonPos + 1);
            
            // Trim whitespace
            key.erase(0, key.find_first_not_of(" \t"));
            key.erase(key.find_last_not_of(" \t\r") + 1);
            value.erase(0, value.find_first_not_of(" \t"));
            value.erase(value.find_last_not_of(" \t\r") + 1);
            
            request.headers[key] = value;
        }
    }
    
    // Parse body (if present)
    std::string body;
    std::string bodyLine;
    while (std::getline(iss, bodyLine)) {
        body += bodyLine + "\n";
    }
    if (!body.empty()) {
        body.pop_back(); // Remove last newline
    }
    request.body = body;
    
    return true;
}

std::string HttpServer::buildResponse(const HttpResponse& response) {
    std::ostringstream oss;
    oss << "HTTP/1.1 " << response.statusCode << " " << response.statusText << "\r\n";
    
    // Add default headers
    oss << "Content-Length: " << response.body.length() << "\r\n";
    oss << "Connection: close\r\n";
    oss << "Access-Control-Allow-Origin: *\r\n";
    oss << "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n";
    oss << "Access-Control-Allow-Headers: Content-Type\r\n";
    
    // Add custom headers
    for (const auto& header : response.headers) {
        oss << header.first << ": " << header.second << "\r\n";
    }
    
    oss << "\r\n" << response.body;
    return oss.str();
}

HttpResponse HttpServer::handleGetRoot() {
    std::string html = loadWebFile("web/index.html");
    if (html.empty()) {
        return HttpResponse(404, "Not Found");
    }
    
    HttpResponse response;
    response.headers["Content-Type"] = "text/html";
    response.body = html;
    return response;
}

HttpResponse HttpServer::handleGetAsset(const std::string& path) {
    std::string filename = "web" + path;
    std::string content = loadWebFile(filename);
    
    if (content.empty()) {
        HttpResponse response(404, "Not Found");
        response.body = "404 - File not found";
        return response;
    }
    
    HttpResponse response;
    response.headers["Content-Type"] = getContentType(path);
    response.body = content;
    return response;
}

HttpResponse HttpServer::handleGetMessages() {
    auto messages = messageHandler.getAllMessages();
    
    nlohmann::json jsonArray;
    for (const auto& msg : messages) {
        nlohmann::json msgJson;
        msgJson["id"] = msg.id;
        msgJson["user"] = msg.user;
        msgJson["message"] = msg.message;
        msgJson["timestamp"] = msg.timestamp;
        jsonArray.push_back(msgJson);
    }
    
    HttpResponse response;
    response.headers["Content-Type"] = "application/json";
    response.body = jsonArray.dump();
    return response;
}

HttpResponse HttpServer::handlePostMessages(const std::string& body) {
    try {
        nlohmann::json json = nlohmann::json::parse(body);
        
        if (!json.contains("user") || !json.contains("message")) {
            HttpResponse response(400, "Bad Request");
            response.body = "Missing user or message field";
            return response;
        }
        
        Message msg;
        msg.user = json["user"];
        msg.message = json["message"];
        
        messageHandler.addMessage(msg);
        
        HttpResponse response(201, "Created");
        response.headers["Content-Type"] = "application/json";
        response.body = "{\"status\":\"success\"}";
        return response;
        
    } catch (const std::exception& e) {
        HttpResponse response(400, "Bad Request");
        response.body = "Invalid JSON: " + std::string(e.what());
        return response;
    }
}

HttpResponse HttpServer::handleGetPeers() {
    auto peers = peerDiscovery.getActivePeers();
    
    nlohmann::json jsonArray;
    for (const auto& peer : peers) {
        nlohmann::json peerJson;
        peerJson["id"] = peer.id;
        peerJson["address"] = peer.address;
        jsonArray.push_back(peerJson);
    }
    
    HttpResponse response;
    response.headers["Content-Type"] = "application/json";
    response.body = jsonArray.dump();
    return response;
}

std::string HttpServer::getContentType(const std::string& path) {
    size_t dotPos = path.find_last_of('.');
    if (dotPos == std::string::npos) {
        return "text/plain";
    }
    
    std::string ext = path.substr(dotPos + 1);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    
    if (ext == "html" || ext == "htm") return "text/html";
    if (ext == "css") return "text/css";
    if (ext == "js") return "application/javascript";
    if (ext == "json") return "application/json";
    if (ext == "png") return "image/png";
    if (ext == "jpg" || ext == "jpeg") return "image/jpeg";
    if (ext == "gif") return "image/gif";
    if (ext == "svg") return "image/svg+xml";
    
    return "text/plain";
}

std::string HttpServer::loadWebFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        return "";
    }
    
    std::ostringstream oss;
    oss << file.rdbuf();
    return oss.str();
}

std::string HttpServer::urlDecode(const std::string& str) {
    std::string result;
    result.reserve(str.length());
    
    for (size_t i = 0; i < str.length(); ++i) {
        if (str[i] == '%' && i + 2 < str.length()) {
            int hex;
            std::istringstream iss(str.substr(i + 1, 2));
            if (iss >> std::hex >> hex) {
                result += static_cast<char>(hex);
                i += 2;
            } else {
                result += str[i];
            }
        } else if (str[i] == '+') {
            result += ' ';
        } else {
            result += str[i];
        }
    }
    
    return result;
}