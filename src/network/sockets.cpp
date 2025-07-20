#include "sockets.hpp"
#include <iostream>
#include <cstring>

// NetworkUtils implementation
bool NetworkUtils::initialize() {
#ifdef WIN32
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    return result == 0;
#else
    return true; // No initialization needed on Unix-like systems
#endif
}

void NetworkUtils::cleanup() {
#ifdef WIN32
    WSACleanup();
#endif
}

std::string NetworkUtils::getLastError() {
#ifdef WIN32
    int error = WSAGetLastError();
    char* message = nullptr;
    FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                   nullptr, error, 0, (LPSTR)&message, 0, nullptr);
    std::string result = message ? message : "Unknown error";
    if (message) LocalFree(message);
    return result;
#else
    return std::string(strerror(errno));
#endif
}

bool NetworkUtils::setNonBlocking(SOCKET sock, bool nonBlocking) {
#ifdef WIN32
    u_long mode = nonBlocking ? 1 : 0;
    return ioctlsocket(sock, FIONBIO, &mode) == 0;
#else
    int flags = fcntl(sock, F_GETFL, 0);
    if (flags == -1) return false;
    flags = nonBlocking ? (flags | O_NONBLOCK) : (flags & ~O_NONBLOCK);
    return fcntl(sock, F_SETFL, flags) == 0;
#endif
}

bool NetworkUtils::setBroadcast(SOCKET sock, bool broadcast) {
    int value = broadcast ? 1 : 0;
    return setsockopt(sock, SOL_SOCKET, SO_BROADCAST, 
                     (const char*)&value, sizeof(value)) == 0;
}

bool NetworkUtils::setReuseAddr(SOCKET sock, bool reuse) {
    int value = reuse ? 1 : 0;
    return setsockopt(sock, SOL_SOCKET, SO_REUSEADDR,
                     (const char*)&value, sizeof(value)) == 0;
}

// UDPSocket implementation
UDPSocket::UDPSocket() : sock(INVALID_SOCKET), bound(false) {
    NetworkUtils::initialize();
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock == INVALID_SOCKET) {
        std::cerr << "Failed to create UDP socket: " << NetworkUtils::getLastError() << std::endl;
    }
}

UDPSocket::~UDPSocket() {
    close();
}

bool UDPSocket::bind(int port) {
    if (sock == INVALID_SOCKET) return false;
    
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);
    
    NetworkUtils::setReuseAddr(sock, true);
    
    if (::bind(sock, (struct sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
        std::cerr << "Failed to bind UDP socket to port " << port << ": " 
                  << NetworkUtils::getLastError() << std::endl;
        return false;
    }
    
    bound = true;
    return true;
}

bool UDPSocket::sendTo(const std::string& data, const std::string& address, int port) {
    if (sock == INVALID_SOCKET) return false;
    
    struct sockaddr_in destAddr;
    memset(&destAddr, 0, sizeof(destAddr));
    destAddr.sin_family = AF_INET;
    destAddr.sin_port = htons(port);
    
    if (inet_pton(AF_INET, address.c_str(), &destAddr.sin_addr) <= 0) {
        std::cerr << "Invalid address: " << address << std::endl;
        return false;
    }
    
    int result = sendto(sock, data.c_str(), data.length(), 0,
                       (struct sockaddr*)&destAddr, sizeof(destAddr));
    
    return result != SOCKET_ERROR;
}

bool UDPSocket::receiveFrom(std::string& data, std::string& fromAddress, int& fromPort, int timeoutMs) {
    if (sock == INVALID_SOCKET) return false;
    
    // Set timeout using select
    fd_set readSet;
    FD_ZERO(&readSet);
    FD_SET(sock, &readSet);
    
    struct timeval timeout;
    timeout.tv_sec = timeoutMs / 1000;
    timeout.tv_usec = (timeoutMs % 1000) * 1000;
    
    int selectResult = select(sock + 1, &readSet, nullptr, nullptr, &timeout);
    if (selectResult <= 0) {
        return false; // Timeout or error
    }
    
    char buffer[1024];
    struct sockaddr_in fromAddr;
    socklen_t fromLen = sizeof(fromAddr);
    
    int result = recvfrom(sock, buffer, sizeof(buffer) - 1, 0,
                         (struct sockaddr*)&fromAddr, &fromLen);
    
    if (result == SOCKET_ERROR) {
        return false;
    }
    
    buffer[result] = '\0';
    data = std::string(buffer);
    
    char addrStr[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &fromAddr.sin_addr, addrStr, INET_ADDRSTRLEN);
    fromAddress = std::string(addrStr);
    fromPort = ntohs(fromAddr.sin_port);
    
    return true;
}

bool UDPSocket::setBroadcast(bool enable) {
    if (sock == INVALID_SOCKET) return false;
    return NetworkUtils::setBroadcast(sock, enable);
}

bool UDPSocket::setNonBlocking(bool enable) {
    if (sock == INVALID_SOCKET) return false;
    return NetworkUtils::setNonBlocking(sock, enable);
}

void UDPSocket::close() {
    if (sock != INVALID_SOCKET) {
        closesocket(sock);
        sock = INVALID_SOCKET;
        bound = false;
    }
}

// TCPSocket implementation
TCPSocket::TCPSocket() : sock(INVALID_SOCKET), connected(false) {
    NetworkUtils::initialize();
    sock = socket(AF_INET, SOCK_STREAM, 0);
}

TCPSocket::TCPSocket(SOCKET existingSocket) : sock(existingSocket), connected(true) {
}

TCPSocket::~TCPSocket() {
    close();
}

bool TCPSocket::connect(const std::string& address, int port) {
    if (sock == INVALID_SOCKET) return false;
    
    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    
    if (inet_pton(AF_INET, address.c_str(), &serverAddr.sin_addr) <= 0) {
        return false;
    }
    
    if (::connect(sock, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        return false;
    }
    
    connected = true;
    return true;
}

bool TCPSocket::send(const std::string& data) {
    if (sock == INVALID_SOCKET || !connected) return false;
    
    int result = ::send(sock, data.c_str(), data.length(), 0);
    return result != SOCKET_ERROR;
}

bool TCPSocket::receive(std::string& data, int timeoutMs) {
    if (sock == INVALID_SOCKET || !connected) return false;
    
    // Set timeout
    fd_set readSet;
    FD_ZERO(&readSet);
    FD_SET(sock, &readSet);
    
    struct timeval timeout;
    timeout.tv_sec = timeoutMs / 1000;
    timeout.tv_usec = (timeoutMs % 1000) * 1000;
    
    int selectResult = select(sock + 1, &readSet, nullptr, nullptr, &timeout);
    if (selectResult <= 0) {
        return false;
    }
    
    char buffer[1024];
    int result = recv(sock, buffer, sizeof(buffer) - 1, 0);
    
    if (result <= 0) {
        connected = false;
        return false;
    }
    
    buffer[result] = '\0';
    data = std::string(buffer);
    return true;
}

bool TCPSocket::setNonBlocking(bool enable) {
    if (sock == INVALID_SOCKET) return false;
    return NetworkUtils::setNonBlocking(sock, enable);
}

void TCPSocket::close() {
    if (sock != INVALID_SOCKET) {
        closesocket(sock);
        sock = INVALID_SOCKET;
        connected = false;
    }
}

// TCPServer implementation
TCPServer::TCPServer() : listenSock(INVALID_SOCKET), listening(false) {
    NetworkUtils::initialize();
    listenSock = socket(AF_INET, SOCK_STREAM, 0);
}

TCPServer::~TCPServer() {
    close();
}

bool TCPServer::bind(int port) {
    if (listenSock == INVALID_SOCKET) return false;
    
    NetworkUtils::setReuseAddr(listenSock, true);
    
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);
    
    if (::bind(listenSock, (struct sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
        return false;
    }
    
    return true;
}

bool TCPServer::listen(int backlog) {
    if (listenSock == INVALID_SOCKET) return false;
    
    if (::listen(listenSock, backlog) == SOCKET_ERROR) {
        return false;
    }
    
    listening = true;
    return true;
}

TCPSocket* TCPServer::accept() {
    if (listenSock == INVALID_SOCKET || !listening) return nullptr;
    
    struct sockaddr_in clientAddr;
    socklen_t clientLen = sizeof(clientAddr);
    
    SOCKET clientSock = ::accept(listenSock, (struct sockaddr*)&clientAddr, &clientLen);
    
    if (clientSock == INVALID_SOCKET) {
        return nullptr;
    }
    
    return new TCPSocket(clientSock);
}

void TCPServer::close() {
    if (listenSock != INVALID_SOCKET) {
        closesocket(listenSock);
        listenSock = INVALID_SOCKET;
        listening = false;
    }
}