#pragma once
#include <nlohmann/json.hpp>
#include <string>
namespace backend::common {
nlohmann::json ParseOutput(const std::string &text);
// 提取尚未生成完整 JSON 中已经完整解码的字符串前缀，不展示原始 JSON。
std::string VisibleField(const std::string &text, const std::string &field);
} // namespace backend::common
