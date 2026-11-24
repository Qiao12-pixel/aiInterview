#pragma once
#include "common/random.h"
#include "common/types.h"
#include <chrono>
#include <mutex>
#include <unordered_map>
namespace gateway {
class Registry {
    struct Entry {
        std::string id;
        std::chrono::steady_clock::time_point expires;
    };
    std::mutex mutex_;
    std::unordered_map<std::string, Entry> entries_;

  public:
    std::string Issue(const std::string &id) {
        std::lock_guard<std::mutex> lock(mutex_);
        const auto now = std::chrono::steady_clock::now();
        for (auto i = entries_.begin(); i != entries_.end();)
            if (i->second.expires < now)
                i = entries_.erase(i);
            else
                ++i;
        if (entries_.size() >= 1000)
            throw Error(503, "网关会话容量已满");
        const auto token = RandomToken();
        entries_[token] = {id, now + std::chrono::hours(2)};
        return token;
    }
    bool Owns(const std::string &token, const std::string &id) {
        std::lock_guard<std::mutex> lock(mutex_);
        const auto found = entries_.find(token);
        return found != entries_.end() && found->second.id == id &&
               found->second.expires > std::chrono::steady_clock::now();
    }
};
} // namespace gateway
