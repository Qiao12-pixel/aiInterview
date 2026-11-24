#include "interview/infrastructure/llm/stream.h"
#include <stdexcept>
namespace backend::llm {
void StreamDecoder::Feed(const std::string &bytes) {
    buffer_ += bytes;
    if (buffer_.size() + content_.size() > 2 * 1024 * 1024)
        throw std::runtime_error("模型响应过大");
    size_t newline;
    while ((newline = buffer_.find('\n')) != std::string::npos) {
        auto line = buffer_.substr(0, newline);
        buffer_.erase(0, newline + 1);
        if (!line.empty() && line.back() == '\r')
            line.pop_back();
        if (line.rfind("data:", 0) != 0)
            continue;
        auto data = line.substr(5);
        if (!data.empty() && data[0] == ' ')
            data.erase(0, 1);
        if (data == "[DONE]") {
            done_ = true;
            continue;
        }
        if (data.empty())
            continue;
        const auto event = nlohmann::json::parse(data);
        if (event.contains("error"))
            throw std::runtime_error("模型流返回错误");
        if (!event.contains("choices") || event["choices"].empty())
            continue;
        const auto &choice = event["choices"][0];
        if (choice.value("finish_reason", nlohmann::json{}) == "length")
            throw std::runtime_error("模型输出被截断，请减少题目或回答长度");
        if (choice.contains("delta") && choice["delta"].contains("content") &&
            choice["delta"]["content"].is_string()) {
            content_ += choice["delta"]["content"].get<std::string>();
            if (sink_)
                sink_(content_);
        }
    }
}
} // namespace backend::llm
