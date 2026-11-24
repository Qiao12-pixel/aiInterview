#pragma once
#include <nlohmann/json.hpp>
#include <mutex>
#include <chrono>
#include <string>
#include <vector>
namespace backend {
using Json = nlohmann::json;
struct Session {
    std::mutex mutex;
    std::chrono::steady_clock::time_point accessed = std::chrono::steady_clock::now();
    std::string id, candidate, current, owner;
    Json questions, records = Json::array(), report;
    size_t index = 0;
    bool followup = false, completed = false;
};
struct Event {
    std::string type, text;
    Json payload;
};
} // namespace backend
