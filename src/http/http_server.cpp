// src/http/http_server.cpp

#include "http_server.hpp"
#include <iostream>
#include <sstream>
#include <nlohmann/json.hpp>
#include <fstream>  // For file loading
#include "../util/utils.hpp"
#include <chrono>

using json = nlohmann::json;

HttpServer::HttpServer(int p, MessageHandler& mh, PeerDiscovery& pd)
    : port(p), msgHandler(mh), peerDisc(pd) {}

HttpServer::~HttpServer() {
    stop();
}

void HttpServer::start() {
    if (running) return;
    running = true;
    serverTh = std::thread(&HttpServer::serverLoop, this);
}

void HttpServer::stop() {
    if (!running) return;
    running = false;
    tcpServer.close();
    if (serverTh.joinable()) serverTh.join();
}

void HttpServer::serverLoop() {
    if (!tcpServer.listen(port)) {
        std::cerr << "Failed to start HTTP server on port " << port << std::endl;
        running = false;
        return;
    }
    std::cout << "HTTP server listening on port " << port << std::endl;
    while (running) {
        sockaddr_in clientAddr;
        SOCKET clientSock = tcpServer.acceptClient(clientAddr);
        if (clientSock != INVALID_SOCKET) {
            std::thread(&HttpServer::handleClient, this, clientSock).detach();
        }
    }
}

void HttpServer::handleClient(SOCKET clientSock) {
    std::cout << "[DEBUG] Handling new client connection" << std::endl;
    TCPSocket client(clientSock);

    std::string rawRequest;
    std::string buffer;
    auto startTime = std::chrono::steady_clock::now();
    bool hasBody = false;
    size_t contentLength = 0;
    while (true) {
        if (!client.receive(buffer, 1024)) break;
        rawRequest += buffer;

        size_t headerEnd = rawRequest.find("\r\n\r\n");
        if (headerEnd != std::string::npos) {
            auto headersStr = rawRequest.substr(0, headerEnd);
            auto contentLenPos = headersStr.find("Content-Length:");
            if (contentLenPos != std::string::npos) {
                std::string lenStr = headersStr.substr(contentLenPos + 15);
                contentLength = std::stoul(lenStr.substr(0, lenStr.find("\r\n")));
                hasBody = true;
            }
            if (!hasBody || rawRequest.size() >= headerEnd + 4 + contentLength) break;
        }

        auto elapsed = std::chrono::steady_clock::now() - startTime;
        if (std::chrono::duration_cast<std::chrono::seconds>(elapsed).count() > 5) {
            std::cerr << "[ERROR] Request timeout" << std::endl;
            break;
        }
    }

    if (rawRequest.empty()) {
        std::cerr << "[ERROR] Empty request" << std::endl;
        client.close();
        return;
    }

    HttpRequest req = parseRequest(rawRequest);

    std::string response;
    try {
        if (req.method == "GET") {
            response = handleGet(req);
        } else if (req.method == "POST") {
            response = handlePost(req);
        } else {
            response = buildResponse("Method Not Allowed", "text/plain", 405);
        }
    } catch (const std::exception& e) {
        std::cerr << "[ERROR] Exception in handler: " << e.what() << std::endl;
        response = buildResponse("Internal Server Error", "text/plain", 500);
    }

    if (!client.send(response)) {
        std::cerr << "[ERROR] Failed to send response" << std::endl;
    }
    client.close();
    std::cout << "[DEBUG] Client handled successfully" << std::endl;
}

HttpRequest HttpServer::parseRequest(const std::string& raw) {
    std::cout << "[DEBUG] Parsing request: " << raw.substr(0, 100) << "..." << std::endl;
    HttpRequest req;
    std::istringstream stream(raw);
    std::string line;

    std::getline(stream, line);
    auto parts = Utils::split(line, ' ');
    if (parts.size() >= 2) {
        req.method = parts[0];
        req.path = parts[1];
    }

    while (std::getline(stream, line) && line != "\r") {
        if (line.empty()) break;
        auto headerParts = Utils::split(line, ':');
        if (headerParts.size() >= 2) {
            std::string key = Utils::trim(headerParts[0]);
            std::string value = Utils::trim(headerParts[1]);
            req.headers[key] = value;
        }
    }

    std::string body;
    while (std::getline(stream, line)) {
        body += line + "\n";
    }
    req.body = Utils::trim(body);
    return req;
}

std::string HttpServer::buildResponse(const std::string& body, const std::string& contentType, int code) {
    std::ostringstream res;
    res << "HTTP/1.1 " << code << " " << (code == 200 ? "OK" : "Error") << "\r\n";
    res << "Content-Type: " << contentType << "\r\n";
    res << "Content-Length: " << body.size() << "\r\n";
    res << "Access-Control-Allow-Origin: *" << "\r\n";
    res << "Access-Control-Allow-Methods: GET, POST" << "\r\n";
    res << "Access-Control-Allow-Headers: Content-Type" << "\r\n";
    res << "Connection: close\r\n\r\n";
    res << body;
    return res.str();
}

std::string HttpServer::handleGet(const HttpRequest& req) {
    std::cout << "[DEBUG] Handling GET for path: " << req.path << std::endl;

    // Serve files from web/ folder
    if (req.path == "/" || req.path == "/index.html") {
        std::ifstream file("web/index.html");
        if (file) {
            std::stringstream buffer;
            buffer << file.rdbuf();
            return buildResponse(buffer.str(), "text/html", 200);
        } else {
            std::cerr << "[ERROR] web/index.html not found" << std::endl;
            return buildResponse("Index file not found", "text/plain", 404);
        }
    } else if (req.path == "/style.css") {
        std::ifstream file("web/style.css");
        if (file) {
            std::stringstream buffer;
            buffer << file.rdbuf();
            return buildResponse(buffer.str(), "text/css", 200);
        } else {
            return buildResponse("CSS file not found", "text/plain", 404);
        }
    } else if (req.path == "/app.js") {
        std::ifstream file("web/app.js");
        if (file) {
            std::stringstream buffer;
            buffer << file.rdbuf();
            return buildResponse(buffer.str(), "application/javascript", 200);
        } else {
            return buildResponse("JS file not found", "text/plain", 404);
        }
    } else if (req.path == "/favicon.ico") {
        std::ifstream file("web/favicon.ico", std::ios::binary);
        if (file) {
            std::stringstream buffer;
            buffer << file.rdbuf();
            return buildResponse(buffer.str(), "image/x-icon", 200);
        } else {
            return buildResponse("", "image/x-icon", 204);  // Empty to silence browser warnings
        }
    } else if (req.path == "/messages") {
        json j = json::array();
        for (const auto& m : msgHandler.getAllMessages()) {
            j.push_back({
                {"id", m.id},
                {"user", m.user},
                {"message", m.message},
                {"timestamp", m.timestamp}
            });
        }
        std::string jsonStr = j.dump(4);
        std::cout << "[DEBUG] Returning messages JSON (size: " << jsonStr.size() << ")" << std::endl;
        return buildResponse(jsonStr, "application/json", 200);
    } else if (req.path == "/peers") {
        json j = json::array();
        for (const auto& p : peerDisc.getActivePeers()) {
            j.push_back({
                {"id", p.id},
                {"address", p.address}
            });
        }
        std::string jsonStr = j.dump(4);
        std::cout << "[DEBUG] Returning peers JSON (size: " << jsonStr.size() << ")" << std::endl;
        return buildResponse(jsonStr, "application/json", 200);
    }
    std::cout << "[DEBUG] 404 for path: " << req.path << std::endl;
    return buildResponse("Not Found", "text/plain", 404);
}

std::string HttpServer::handlePost(const HttpRequest& req) {
    std::cout << "[DEBUG] Handling POST for path: " << req.path << " with body: " << req.body.substr(0, 100) << "..." << std::endl;
    if (req.path == "/messages") {
        try {
            json parsed = json::parse(req.body);
            std::string user = parsed.value("user", "anonymous");
            std::string message = parsed.value("message", "");
            if (!message.empty()) {
                msgHandler.addMessage(user, message);
                std::cout << "[DEBUG] Message added successfully" << std::endl;
                return buildResponse("{\"status\": \"ok\"}", "application/json", 200);
            } else {
                std::cout << "[DEBUG] Missing message in POST" << std::endl;
                return buildResponse("{\"error\": \"Missing message\"}", "application/json", 400);
            }
        } catch (const std::exception& e) {
            std::cerr << "[ERROR] POST parse error: " << e.what() << std::endl;
            return buildResponse("{\"error\": \"Invalid JSON\"}", "application/json", 400);
        }
    } else if (req.path == "/clear") {
        msgHandler.clear();
        std::cout << "[DEBUG] Messages cleared" << std::endl;
        return buildResponse("{\"status\": \"cleared\"}", "application/json", 200);
    }
    std::cout << "[DEBUG] 404 for POST path: " << req.path << std::endl;
    return buildResponse("Not Found", "text/plain", 404);
}