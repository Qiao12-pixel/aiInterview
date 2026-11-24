#include "application/engine.h"
#include "common/json_output.h"
#include <random>
#include <sstream>
#include <iomanip>
#include <stdexcept>
namespace backend {
std::shared_ptr<Session> Engine::Create(const std::string &candidate, unsigned count,
                                        const std::string &resume_text, const std::string &owner,
                                        const EventSink &sink) {
    if (candidate.empty() || candidate.size() > 120 || count < 1 || count > 10)
        throw std::invalid_argument("姓名必填，题目数量须为 1–10");
    auto s = std::make_shared<Session>();
    s->candidate = candidate;
    s->owner = owner;
    TokenSink preview;
    if (sink)
        preview = [&, last = std::string{}](const std::string &json) mutable {
            const auto question = common::VisibleField(json, "question");
            if (question != last) {
                last = question;
                sink({"DELTA", question, {{"target", "QUESTION"}}});
            }
        };
    s->questions = model_("questions", {{"count", count}, {"resume_text", resume_text}}, preview);
    if (!s->questions.is_array() || s->questions.size() != count)
        throw std::runtime_error("模型返回的题目数量不符合要求");
    for (const auto &q : s->questions) {
        if (!q.contains("question") || !q["question"].is_string() ||
            q["question"].get<std::string>().empty())
            throw std::runtime_error("模型返回了无效题目");
    }
    s->current = s->questions[0]["question"];
    std::lock_guard<std::mutex> lock(mutex_);
    if (sessions_.size() >= 1000)
        throw std::runtime_error("会话容量已满，请稍后重试");
    do {
        std::random_device random;
        std::ostringstream id;
        for (int i = 0; i < 4; ++i)
            id << std::hex << std::setw(8) << std::setfill('0') << random();
        s->id = id.str();
    } while (sessions_.count(s->id));
    sessions_[s->id] = s;
    return s;
}
std::shared_ptr<Session> Engine::Find(const std::string &id) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = sessions_.find(id);
    return it == sessions_.end() ? nullptr : it->second;
}
} // namespace backend
