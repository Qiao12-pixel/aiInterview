#include "infrastructure/messaging/report_document.h"
namespace backend::messaging {
std::string RenderReportDocument(const nlohmann::json &report) {
    std::string out =
        "# " + report.value("candidate_name", std::string("候选人")) + " · 面试报告\n\n";
    if (report.contains("overall_score"))
        out += "综合评分：" + report["overall_score"].dump() + " / 100\n\n";
    out += report.value("summary", std::string{}) + "\n\n";
    unsigned index = 0;
    for (const auto &record : report.value("records", nlohmann::json::array())) {
        out +=
            "## 第 " + std::to_string(++index) + " 轮\n\n面试官：" +
            record.value("question", std::string{}) + "\n\n回答：" +
            record.value("answer", std::string{}) + "\n\n评价：" +
            record.value("evaluation", nlohmann::json::object()).value("feedback", std::string{}) +
            "\n\n";
    }
    return out;
}
} // namespace backend::messaging
