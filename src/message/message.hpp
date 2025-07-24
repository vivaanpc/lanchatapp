#pragma once

#include <string>

struct Message {
    std::string id;
    std::string user;
    std::string message;
    std::string timestamp;

    Message() = default;
    Message(const std::string& u, const std::string& m) : user(u), message(m) {}

    void generateId();
    void generateTimestamp();
};
