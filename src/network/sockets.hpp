#pragma once
#include <string>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#define CLOSE_SOCKET closesocket
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#define CLOSE_SOCKET close
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
typedef int SOCKET;
#endif

class SocketUtils {
public:
    static bool initialize();
    static void cleanup();
};

class UDPSocket {
public:
    UDPSocket();
    ~UDPSocket();
    bool bind(int port);
    bool sendTo(const std::string& data, const std::string& ip, int port);
    bool receiveFrom(std::string& data, std::string& ip, int& port, int timeoutMs = 1000);
    bool setBroadcast(bool enable);
    void close();
private:
    SOCKET sock = INVALID_SOCKET;
};

class TCPServer {
public:
    TCPServer();
    ~TCPServer();
    bool listen(int port, int backlog = 5);
    SOCKET acceptClient(sockaddr_in& clientAddr);
    void close();
private:
    SOCKET listenSock = INVALID_SOCKET;
};

class TCPSocket {
public:
    TCPSocket(SOCKET s);
    ~TCPSocket();
    bool send(const std::string& data);
    bool receive(std::string& data, size_t maxLen = 4096);
    void close();
private:
    SOCKET sock;
};