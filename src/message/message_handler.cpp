// src/message/message_handler.cpp

#include "message_handler.hpp"
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <exception>  // For std::exception in catch blocks
#include <mutex>      // For std::lock_guard

using json = nlohmann::json;

MessageHandler::MessageHandler(const std::string& file) : filename(file) {
    loadFromFile();
}

MessageHandler::~MessageHandler() {
    saveToFile();
}

void MessageHandler::addMessage(const std::string& user, const std::string& text) {
    Message msg(user, text);
    msg.generateId();
    msg.generateTimestamp();
    std::lock_guard<std::mutex> lock(mutex);
    messages.push_back(msg);
    saveToFile();
}

void MessageHandler::addMessage(const Message& msg) {
    std::lock_guard<std::mutex> lock(mutex);
    messages.push_back(msg);
    saveToFile();
}

std::vector<Message> MessageHandler::getAllMessages() const {
    std::lock_guard<std::mutex> lock(mutex);
    return messages;
}

void MessageHandler::clear() {
    std::lock_guard<std::mutex> lock(mutex);
    messages.clear();
    saveToFile();
}

void MessageHandler::loadFromFile() {
    std::lock_guard<std::mutex> lock(mutex);
    std::ifstream file(filename);
    if (!file.is_open()) return;
    try {
        json j;
        file >> j;
        messages.clear();
        for (const auto& item : j) {
            Message msg;
            msg.id = item.value("id", "");
            msg.user = item.value("user", "");
            msg.message = item.value("message", "");
            msg.timestamp = item.value("timestamp", "");
            messages.push_back(msg);
        }
    } catch (const std::exception& e) {
        std::cerr << "Error loading messages: " << e.what() << "\n";
    }
}

void MessageHandler::saveToFile() const {
    std::lock_guard<std::mutex> lock(mutex);
    std::ofstream file(filename);
    if (!file.is_open()) return;
    try {
        json j = json::array();
        for (const auto& m : messages) {
            j.push_back({
                {"id", m.id},
                {"user", m.user},
                {"message", m.message},
                {"timestamp", m.timestamp}
            });
        }
        file << j.dump(4);
    } catch (const std::exception& e) {
        std::cerr << "Error saving messages: " << e.what() << "\n";
    }
}
