#include "interview/engine.h"
#include "interview/infrastructure/llm.h"
#include <stdexcept>
#include <iostream>
void check(bool value) {
    if (!value)
        throw std::runtime_error("test failed");
}
int main() {
    backend::Engine engine(backend::MakeModel("", true));
    auto s = engine.Create("测试候选人", 2);
    check(engine.Find(s->id) == s);
    auto first = engine.Answer(*s, "RAII 使用对象生命周期管理资源");
    check(first.back().type == "QUESTION" && s->index == 1);
    auto last = engine.Answer(*s, "析构函数释放资源");
    check(last.back().type == "SUMMARY" && s->completed);
    try {
        engine.Answer(*s, "重复提交");
        return 1;
    } catch (const std::invalid_argument &) {
    }
    bool fail = false;
    backend::Engine followup([&](const std::string &op, const backend::Json &input,
                                 const backend::TokenSink &) -> backend::Json {
        if (op == "questions") {
            check(input["resume_text"] == "简历内容");
            return backend::Json::array({{{"question", "主问题"}}});
        }
        if (op == "summary") {
            if (fail)
                throw std::runtime_error("model down");
            return {{"summary", "完成"}};
        }
        return {{"score", 60},
                {"feedback", "继续追问"},
                {"need_followup", true},
                {"followup_question", "追问内容"}};
    });
    auto f = followup.Create("测试", 1, "简历内容");
    check(followup.Answer(*f, "初次回答").back().type == "FOLLOW_UP");
    fail = true;
    try {
        followup.Answer(*f, "追问回答");
        return 1;
    } catch (const std::runtime_error &) {
    }
    check(f->records.size() == 1 && !f->completed);
    fail = false;
    check(followup.Answer(*f, "追问回答").back().type == "SUMMARY");
    check(f->records[1]["question"] == "追问内容");
    std::cout << "state transitions and failure rollback passed\n";
}
