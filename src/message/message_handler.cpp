#include "message_handler.hpp"
#include "json.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <chrono>
#include <iomanip>
#include <random>
#include <algorithm>

#ifdef WIN32
    #include <direct.h>
    #define mkdir(path, mode) _mkdir(path)
#else
    #include <sys/stat.h>
    #include <sys/types.h>
#endif

void Message::generateId() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(100000, 999999);
    
    id = std::to_string(dis(gen));
}

void Message::generateTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    
    std::ostringstream oss;
    oss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    timestamp = oss.str();
}

MessageHandler::MessageHandler(const std::string& dataFile) : filename(dataFile) {
    ensureDataDirectory();
    loadMessages();
}

MessageHandler::~MessageHandler() {
    saveMessages();
}

void MessageHandler::ensureDataDirectory() {
    // Extract directory from filename
    size_t lastSlash = filename.find_last_of("/\\");
    if (lastSlash != std::string::npos) {
        std::string dir = filename.substr(0, lastSlash);
        
        // Create directory if it doesn't exist
        mkdir(dir.c_str(), 0755);
    }
}

void MessageHandler::loadMessages() {
    std::lock_guard<std::mutex> lock(messagesMutex);
    
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cout << "No existing message file found. Starting fresh." << std::endl;
        return;
    }
    
    try {
        nlohmann::json json;
        file >> json;
        
        if (json.contains("messages") && json["messages"].is_array()) {
            for (const auto& msgJson : json["messages"]) {
                Message msg;
                if (msgJson.contains("id")) msg.id = msgJson["id"];
                if (msgJson.contains("user")) msg.user = msgJson["user"];
                if (msgJson.contains("message")) msg.message = msgJson["message"];
                if (msgJson.contains("timestamp")) msg.timestamp = msgJson["timestamp"];
                
                messages.push_back(msg);
            }
            
            std::cout << "Loaded " << messages.size() << " messages from " << filename << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error loading messages: " << e.what() << std::endl;
        messages.clear();
    }
}

void MessageHandler::saveMessages() {
    std::lock_guard<std::mutex> lock(messagesMutex);
    
    try {
        nlohmann::json json;
        nlohmann::json messagesArray;
        
        for (const auto& msg : messages) {
            nlohmann::json msgJson;
            msgJson["id"] = msg.id;
            msgJson["user"] = msg.user;
            msgJson["message"] = msg.message;
            msgJson["timestamp"] = msg.timestamp;
            messagesArray.push_back(msgJson);
        }
        
        json["messages"] = messagesArray;
        
        std::ofstream file(filename);
        if (file.is_open()) {
            file << json.dump(2); // Pretty print with 2 spaces
            file.close();
            std::cout << "Saved " << messages.size() << " messages to " << filename << std::endl;
        } else {
            std::cerr << "Failed to open file for writing: " << filename << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error saving messages: " << e.what() << std::endl;
    }
}

void MessageHandler::addMessage(const Message& message) {
    std::lock_guard<std::mutex> lock(messagesMutex);
    
    Message msg = message;
    if (msg.id.empty()) {
        msg.generateId();
    }
    if (msg.timestamp.empty()) {
        msg.generateTimestamp();
    }
    
    messages.push_back(msg);
    
    // Trim messages if we exceed the limit
    if (messages.size() > MAX_MESSAGES) {
        messages.erase(messages.begin(), messages.begin() + (messages.size() - MAX_MESSAGES));
    }
    
    // Save immediately to ensure persistence
    // Note: We're already holding the lock, so we need a version that doesn't lock again
    try {
        nlohmann::json json;
        nlohmann::json messagesArray;
        
        for (const auto& m : messages) {
            nlohmann::json msgJson;
            msgJson["id"] = m.id;
            msgJson["user"] = m.user;
            msgJson["message"] = m.message;
            msgJson["timestamp"] = m.timestamp;
            messagesArray.push_back(msgJson);
        }
        
        json["messages"] = messagesArray;
        
        std::ofstream file(filename);
        if (file.is_open()) {
            file << json.dump(2);
            file.close();
        }
    } catch (const std::exception& e) {
        std::cerr << "Error saving message: " << e.what() << std::endl;
    }
}

void MessageHandler::addMessage(const std::string& user, const std::string& text) {
    Message msg(user, text);
    addMessage(msg);
}

std::vector<Message> MessageHandler::getAllMessages() const {
    std::lock_guard<std::mutex> lock(messagesMutex);
    return messages;
}

std::vector<Message> MessageHandler::getRecentMessages(size_t count) const {
    std::lock_guard<std::mutex> lock(messagesMutex);
    
    if (count >= messages.size()) {
        return messages;
    }
    
    return std::vector<Message>(messages.end() - count, messages.end());
}

size_t MessageHandler::getMessageCount() const {
    std::lock_guard<std::mutex> lock(messagesMutex);
    return messages.size();
}

void MessageHandler::clearMessages() {
    std::lock_guard<std::mutex> lock(messagesMutex);
    messages.clear();
    
    // Save empty file
    try {
        nlohmann::json json;
        json["messages"] = nlohmann::json::array();
        
        std::ofstream file(filename);
        if (file.is_open()) {
            file << json.dump(2);
            file.close();
        }
    } catch (const std::exception& e) {
        std::cerr << "Error clearing messages: " << e.what() << std::endl;
    }
}