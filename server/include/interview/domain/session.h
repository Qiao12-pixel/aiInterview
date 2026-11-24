#pragma once
#include <nlohmann/json.hpp>
#include <mutex>
#include <string>
#include <vector>
namespace backend {
using Json = nlohmann::json;
struct Session {
    std::mutex mutex;
    std::string id, candidate, current;
    Json questions, records = Json::array(), report;
    size_t index = 0;
    bool followup = false, completed = false;
};
struct Event {
    std::string type, text;
    Json payload;
};
} // namespace backend
