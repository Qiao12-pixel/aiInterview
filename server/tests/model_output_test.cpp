#include "interview/common/json_output.h"
#include "interview/common/base64.h"
#include "interview/infrastructure/llm/stream.h"
#include <iostream>
#include <stdexcept>
void check(bool value) {
    if (!value)
        throw std::runtime_error("model output test failed");
}
int main() {
    using namespace backend;
    const auto output = common::ParseOutput(
        "```json\n{\"feedback\":\"代码中有 } 和 {\",\"score\":80}\n```\n解释 {非法}");
    check(output["score"] == 80);
    check(common::VisibleField("{\"feedback\":\"你好\\n世界", "feedback") == "你好\n世界");
    check(common::VisibleField("{\"feedback\":\"你好\\u4", "feedback") == "你好");
    check(common::VisibleField("{\"feedback\":\"你好\\ud83d\\ude", "feedback") == "你好");
    check(common::Base64("abc") == "YWJj" && common::Base64("a") == "YQ==");
    const auto content = nlohmann::json{{"feedback", "流式输出"}}.dump();
    std::string wire;
    // 实际 token 按完整 UTF-8 提供，再把传输字节拆为任意网络分片。
    wire = "data: " +
           nlohmann::json{{"choices", nlohmann::json::array({{{"delta", {{"content", content}}}}})}}
               .dump() +
           "\r\n\r\ndata: [DONE]\n\n";
    std::string preview;
    llm::StreamDecoder decoder(
        [&](const std::string &text) { preview = common::VisibleField(text, "feedback"); });
    for (char ch : wire)
        decoder.Feed(std::string(1, ch));
    check(decoder.Complete() && decoder.Content() == content && preview == "流式输出");
    bool rejected = false;
    try {
        llm::StreamDecoder broken({});
        broken.Feed("data: {bad}\n\n");
    } catch (const std::exception &) {
        rejected = true;
    }
    check(rejected);
    std::cout << "JSON boundaries, Unicode prefixes and SSE network chunks passed\n";
}
