#include "infrastructure/llm.h"
namespace backend {
using Json = nlohmann::json;
Model MakeMockModel() {
    return [](const std::string &operation, const Json &input, const TokenSink &sink) -> Json {
        if (operation == "questions") {
            Json list = Json::array();
            for (unsigned i = 0; i < input.at("count").get<unsigned>(); ++i)
                list.push_back({{"question", "请解释 RAII 如何保证 C++ 资源安全？（测试题 " +
                                                 std::to_string(i + 1) + "）"}});
            if (sink) {
                const auto text = Json{{"questions", list}}.dump();
                for (size_t i = 1; i <= text.size(); ++i)
                    sink(text.substr(0, i));
            }
            return list;
        }
        Json result = operation == "evaluate"
                          ? Json{{"score", 80},
                                 {"feedback", "测试评分：已收到回答"},
                                 {"need_followup", false}}
                          : Json{{"summary", "测试面试已完成"}, {"overall_score", 80}};
        if (sink) {
            const auto content = result.dump();
            for (size_t i = 1; i <= content.size(); ++i)
                sink(content.substr(0, i));
        }
        return result;
    };
}
} // namespace backend
