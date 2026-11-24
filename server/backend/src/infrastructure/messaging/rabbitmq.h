#pragma once
#include <nlohmann/json.hpp>
#include <memory>
#include <functional>
namespace backend::messaging {
class Publisher {
    class Impl;
    std::unique_ptr<Impl> impl_;

  public:
    explicit Publisher(const nlohmann::json &config);
    ~Publisher();
    bool ConsumeOnce(const std::function<void(const std::string &, const std::string &)> &handler);
    void Publish(const std::string &id, const std::string &payload);
};
} // namespace backend::messaging
