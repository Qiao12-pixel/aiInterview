#include "infrastructure/llm/config.h"
#include <fstream>
#include <stdexcept>
namespace backend::llm {
Config LoadConfig(const std::string &path) {
    std::ifstream file(path);
    nlohmann::json root;
    if (!file)
        throw std::runtime_error("无法读取服务端配置文件");
    file >> root;
    const auto &value = root.at("llm");
    Config result;
    result.url = value.at("api_url");
    result.key = value.at("api_key");
    result.model = value.at("model");
    result.thinking = value.value("thinking_type", std::string{});
    result.max_tokens = value.value("max_tokens", 4096);
    result.timeout = value.value("timeout_seconds", 120);
    result.temperature = value.value("temperature", 0.3);
    result.json_mode = value.value("json_mode", true);
    if (result.url.rfind("https://", 0) != 0 || result.key.empty() || result.model.empty())
        throw std::runtime_error("模型配置无效：需要 HTTPS 地址、Key 和模型 ID");
    if (result.max_tokens < 1 || result.timeout < 1 || result.timeout > 120 ||
        result.temperature < 0 || result.temperature > 2)
        throw std::runtime_error("模型参数无效：超时须为 1–120 秒、temperature 为 0–2");
    return result;
}
} // namespace backend::llm
