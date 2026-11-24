#include "interview/engine.h"
#include "interview/common/json_output.h"
#include <stdexcept>
namespace backend {
std::vector<Event> Engine::Answer(Session &s, const std::string &text, const EventSink &sink,
                                  const std::string &frame) {
    // 调用方持有会话锁；模型失败时不提交半完成的状态。
    if (s.completed)
        throw std::invalid_argument("面试已经结束");
    if (text.empty() || text.size() > 32000)
        throw std::invalid_argument("回答须为 1–32000 字节");
    auto preview = [&](const std::string &kind, const std::string &field) -> TokenSink {
        if (!sink)
            return {};
        return [&, kind, field, last = std::string{}](const std::string &json) mutable {
            const auto visible = common::VisibleField(json, field);
            if (visible != last) {
                last = visible;
                sink({"DELTA", visible, {{"target", kind}}});
            }
        };
    };
    auto evaluation =
        model_("evaluate", {{"question", s.current}, {"answer", text}, {"video_jpeg", frame}},
               preview("FEEDBACK", "feedback"));
    const int score = evaluation.at("score").get<int>();
    if (score < 0 || score > 100 || !evaluation.at("feedback").is_string())
        throw std::runtime_error("模型评分格式错误");
    auto records = s.records;
    records.push_back({{"question", s.current}, {"answer", text}, {"evaluation", evaluation}});
    std::vector<Event> events{{"FEEDBACK", evaluation["feedback"], evaluation}};
    const auto followup = evaluation.value("followup_question", std::string{});
    if (!s.followup && evaluation.value("need_followup", false) && !followup.empty()) {
        s.records = std::move(records);
        s.followup = true;
        s.current = followup;
        events.push_back({"FOLLOW_UP", s.current, {}});
    } else if (s.index + 1 == s.questions.size()) {
        auto report = model_("summary", records, preview("SUMMARY", "summary"));
        const auto summary = report.at("summary").get<std::string>();
        report["candidate_name"] = s.candidate;
        report["session_id"] = s.id;
        report["total_questions"] = s.questions.size();
        report["records"] = records;
        s.records = std::move(records);
        s.report = std::move(report);
        s.completed = true;
        events.push_back({"SUMMARY", summary, s.report});
    } else {
        s.records = std::move(records);
        ++s.index;
        s.followup = false;
        s.current = s.questions[s.index]["question"];
        events.push_back(
            {"QUESTION",
             s.current,
             {{"question_index", s.index + 1}, {"question_total", s.questions.size()}}});
    }
    return events;
}
} // namespace backend
