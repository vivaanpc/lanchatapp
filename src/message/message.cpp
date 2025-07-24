#include "message.hpp"
#include "../util/utils.hpp"
#include <random>

void Message::generateId() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(100000, 999999);
    id = std::to_string(dis(gen));
}

void Message::generateTimestamp() {
    timestamp = Utils::getCurrentTimeString();
}
