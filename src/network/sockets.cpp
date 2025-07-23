#include "sockets.hpp"
#include <iostream>
#include <cstring>

bool SocketUtils::initialize() {
#ifdef _WIN32
    WSADATA wsaData;
    return WSAStartup(MAKEWORD(2, 2), &wsaData) == 0;
#endif
    return true;
}

void SocketUtils::cleanup() {
#ifdef _WIN32
    WSACleanup();
#endif
}

// UDPSocket
UDPSocket::UDPSocket() {
    sock = socket(AF_INET, SOCK_DGRAM, 0);
}

UDPSocket::~UDPSocket() {
    close();
}

bool UDPSocket::bind(int port) {
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;
    return ::bind(sock, (sockaddr*)&addr, sizeof(addr)) == 0;
}

bool UDPSocket::sendTo(const std::string& data, const std::string& ip, int port) {
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, ip.c_str(), &addr.sin_addr);
    return sendto(sock, data.c_str(), data.size(), 0, (sockaddr*)&addr, sizeof(addr)) != SOCKET_ERROR;
}

bool UDPSocket::receiveFrom(std::string& data, std::string& ip, int& port, int timeoutMs) {
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(sock, &fds);
    timeval tv = {timeoutMs / 1000, (timeoutMs % 1000) * 1000};
    if (select(sock + 1, &fds, nullptr, nullptr, &tv) > 0) {
        char buf[4096];
        sockaddr_in sender;
        socklen_t len = sizeof(sender);
        int bytes = recvfrom(sock, buf, sizeof(buf), 0, (sockaddr*)&sender, &len);
        if (bytes > 0) {
            data = std::string(buf, bytes);
            ip = inet_ntoa(sender.sin_addr);
            port = ntohs(sender.sin_port);
            return true;
        }
    }
    return false;
}

bool UDPSocket::setBroadcast(bool enable) {
    int opt = enable ? 1 : 0;
    return setsockopt(sock, SOL_SOCKET, SO_BROADCAST, (char*)&opt, sizeof(opt)) == 0;
}

void UDPSocket::close() {
    if (sock != INVALID_SOCKET) {
        CLOSE_SOCKET(sock);
        sock = INVALID_SOCKET;
    }
}

// TCPServer
TCPServer::TCPServer() {
    listenSock = socket(AF_INET, SOCK_STREAM, 0);
}

TCPServer::~TCPServer() {
    close();
}

bool TCPServer::listen(int port, int backlog) {
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;
    int opt = 1;
    setsockopt(listenSock, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt));
    if (::bind(listenSock, (sockaddr*)&addr, sizeof(addr)) == 0 && ::listen(listenSock, backlog) == 0) {
        return true;
    }
    return false;
}

SOCKET TCPServer::acceptClient(sockaddr_in& clientAddr) {
    socklen_t len = sizeof(clientAddr);
    return accept(listenSock, (sockaddr*)&clientAddr, &len);
}

void TCPServer::close() {
    if (listenSock != INVALID_SOCKET) {
        CLOSE_SOCKET(listenSock);
        listenSock = INVALID_SOCKET;
    }
}

// TCPSocket
TCPSocket::TCPSocket(SOCKET s) : sock(s) {}

TCPSocket::~TCPSocket() {
    close();
}

bool TCPSocket::send(const std::string& data) {
    return ::send(sock, data.c_str(), data.size(), 0) != SOCKET_ERROR;
}

bool TCPSocket::receive(std::string& data, size_t maxLen) {
    char buf[4096];
    int bytes = ::recv(sock, buf, maxLen, 0);
    if (bytes > 0) {
        data = std::string(buf, bytes);
        return true;
    }
    return false;
}

void TCPSocket::close() {
    if (sock != INVALID_SOCKET) {
        CLOSE_SOCKET(sock);
        sock = INVALID_SOCKET;
    }
}
