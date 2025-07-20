#pragma once

#include <string>
#include <stdexcept>

#ifdef WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
    typedef int socklen_t;
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #include <fcntl.h>
    #define SOCKET int
    #define INVALID_SOCKET -1
    #define SOCKET_ERROR -1
    #define closesocket close
#endif

class NetworkUtils {
public:
    static bool initialize();
    static void cleanup();
    static std::string getLastError();
    static bool setNonBlocking(SOCKET sock, bool nonBlocking);
    static bool setBroadcast(SOCKET sock, bool broadcast);
    static bool setReuseAddr(SOCKET sock, bool reuse);
};

class UDPSocket {
private:
    SOCKET sock;
    bool bound;
    struct sockaddr_in addr;

public:
    UDPSocket();
    ~UDPSocket();
    
    bool bind(int port);
    bool sendTo(const std::string& data, const std::string& address, int port);
    bool receiveFrom(std::string& data, std::string& fromAddress, int& fromPort, int timeoutMs = 1000);
    bool setBroadcast(bool enable);
    bool setNonBlocking(bool enable);
    void close();
    bool isValid() const { return sock != INVALID_SOCKET; }
};

class TCPSocket {
private:
    SOCKET sock;
    bool connected;

public:
    TCPSocket();
    explicit TCPSocket(SOCKET existingSocket);
    ~TCPSocket();
    
    bool connect(const std::string& address, int port);
    bool send(const std::string& data);
    bool receive(std::string& data, int timeoutMs = 5000);
    bool setNonBlocking(bool enable);
    void close();
    bool isValid() const { return sock != INVALID_SOCKET; }
    bool isConnected() const { return connected; }
};

class TCPServer {
private:
    SOCKET listenSock;
    struct sockaddr_in addr;
    bool listening;

public:
    TCPServer();
    ~TCPServer();
    
    bool bind(int port);
    bool listen(int backlog = 5);
    TCPSocket* accept();
    void close();
    bool isListening() const { return listening; }
};