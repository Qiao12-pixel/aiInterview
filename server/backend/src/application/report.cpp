#include "application/engine.h"
#include <stdexcept>
namespace backend {
std::optional<Json> Engine::Report(const std::string &id) {
    if (reports_)
        return reports_->Load(id);
    auto session = Find(id);
    if (!session)
        return std::nullopt;
    std::unique_lock<std::mutex> lock(session->mutex, std::try_to_lock);
    if (!lock.owns_lock())
        throw std::runtime_error("会话正在处理中");
    if (!session->completed)
        throw std::invalid_argument("面试尚未结束");
    return session->report;
}
Json Engine::ListReports(const std::string &owner, unsigned offset) {
    if (!reports_)
        return Json::array();
    return reports_->List(owner, offset);
}
} // namespace backend
