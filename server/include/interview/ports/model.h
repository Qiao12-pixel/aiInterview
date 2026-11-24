#pragma once
#include <nlohmann/json.hpp>
#include <functional>
#include <string>
namespace backend {
// 返回当前累积 JSON 文本，调用者只提取允许展示的字段。
using TokenSink = std::function<void(const std::string &)>;
using Model =
    std::function<nlohmann::json(const std::string &, const nlohmann::json &, const TokenSink &)>;
} // namespace backend
