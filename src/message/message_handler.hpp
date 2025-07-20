#pragma once

#include <string>
#include <vector>
#include <mutex>

struct Message {
    std::string id;
    std::string user;
    std::string message;
    std::string timestamp;
    
    Message() = default;
    Message(const std::string& u, const std::string& m)
        : user(u), message(m) {
        generateId();
        generateTimestamp();
    }
    
private:
    void generateId();
    void generateTimestamp();
};

class MessageHandler {
private:
    std::string filename;
    std::vector<Message> messages;
    std::mutex messagesMutex;
    static const size_t MAX_MESSAGES = 1000; // Prevent file from growing too large
    
    void loadMessages();
    void saveMessages();
    void ensureDataDirectory();
    
public:
    explicit MessageHandler(const std::string& dataFile);
    ~MessageHandler();
    
    void addMessage(const Message& message);
    void addMessage(const std::string& user, const std::string& text);
    
    std::vector<Message> getAllMessages() const;
    std::vector<Message> getRecentMessages(size_t count) const;
    
    size_t getMessageCount() const;
    void clearMessages();
    
    // Disable copy constructor and assignment
    MessageHandler(const MessageHandler&) = delete;
    MessageHandler& operator=(const MessageHandler&) = delete;
};