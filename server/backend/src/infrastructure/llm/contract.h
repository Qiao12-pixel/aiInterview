#pragma once
#include <nlohmann/json.hpp>
#include <string>
namespace backend::llm {
std::string Prompt(const std::string &operation);
void Validate(const nlohmann::json &output, const std::string &operation);
} // namespace backend::llm
