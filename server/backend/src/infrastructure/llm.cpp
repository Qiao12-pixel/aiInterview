#include "infrastructure/llm.h"
#include "infrastructure/llm/config.h"
#include "infrastructure/llm/http.h"
#include "common/json_output.h"
#include <stdexcept>
#include "infrastructure/llm/contract.h"
namespace backend {
using Json = nlohmann::json;
Model MakeModel(const std::string &path, bool mock) {
    if (mock)
        return MakeMockModel();
    const auto config = llm::LoadConfig(path);
    return [config](const std::string &operation, const Json &input,
                    const TokenSink &sink) -> Json {
        auto user = input;
        std::string frame;
        if (user.is_object()) {
            frame = user.value("video_jpeg", std::string{});
            user.erase("video_jpeg");
        }
        Json content = user.dump();
        if (!frame.empty())
            content = Json::array({{{"type", "text"}, {"text", user.dump()}},
                                   {{"type", "image_url"},
                                    {"image_url", {{"url", "data:image/jpeg;base64," + frame}}}}});
        Json body = {
            {"model", config.model},
            {"max_tokens", config.max_tokens},
            {"temperature", config.temperature},
            {"messages", Json::array({{{"role", "system"}, {"content", llm::Prompt(operation)}},
                                      {{"role", "user"}, {"content", content}}})},
            {"stream", static_cast<bool>(sink)}};
        if (!config.thinking.empty())
            body["thinking"] = {{"type", config.thinking}};
        if (config.json_mode)
            body["response_format"] = {{"type", "json_object"}};
        // 只对结构错误重试一次，HTTP/鉴权错误不隐式重试。
        for (int attempt = 0; attempt < 2; ++attempt) {
            if (attempt && sink)
                sink("");
            const auto raw = llm::Call(config, body, sink);
            try {
                const auto output = common::ParseOutput(raw);
                llm::Validate(output, operation);
                return operation == "questions" ? output["questions"] : output;
            } catch (const std::exception &) {
                if (attempt)
                    throw std::runtime_error(
                        "模型结构化输出失败两次，请重试；本次回答未提交到会话。");
                body["messages"].push_back(
                    {{"role", "user"},
                     {"content",
                      "上次返回的结构无效。仅输出符合要求的 JSON，不使用代码围栏或解释。"}});
            }
        }
        throw std::runtime_error("模型输出失败");
    };
}
} // namespace backend
