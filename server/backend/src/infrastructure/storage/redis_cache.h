#pragma once
#include <nlohmann/json.hpp>
#include <optional>
#include <string>
namespace backend::storage {
std::optional<std::string> CacheReport(const nlohmann::json &config, const std::string &id,
                                       const std::string *value);
}
