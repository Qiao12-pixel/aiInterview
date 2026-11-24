#pragma once
#include <nlohmann/json.hpp>
#include <string>
namespace backend::llm {
struct Config {
    std::string url, key, model, thinking;
    int max_tokens = 4096, timeout = 120;
    double temperature = 0.3;
    bool json_mode = true;
};
Config LoadConfig(const std::string &path);
} // namespace backend::llm
