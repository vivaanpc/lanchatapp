#pragma once

#include <string>
#include <vector>
#include <mutex>
#include "message.hpp"

class MessageHandler {
    public:
        explicit MessageHandler(const std::string& file);
        ~MessageHandler();
        void addMessage(const std::string& user, const std::string& text);
        void addMessage(const Message& msg);
        std::vector<Message> getAllMessages() const;
        void clear();  // Added clear method declaration

    private:
        void loadFromFile();
        void saveToFile() const;

        std::string filename;
        std::vector<Message> messages;
        mutable std::mutex mutex;
};
