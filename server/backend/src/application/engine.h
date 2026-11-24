#pragma once
#include "domain/session.h"
#include "ports/model.h"
#include "ports/report_store.h"
#include <mutex>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
namespace backend {
class Engine {
  public:
    explicit Engine(Model model, std::shared_ptr<ReportStore> reports = {},
                    std::string instanceId = {})
        : model_(std::move(model)), reports_(std::move(reports)),
          instance_id_(std::move(instanceId)) {}
    std::optional<Json> Report(const std::string &id);
    Json ListReports(const std::string &owner, unsigned offset);
    using EventSink = std::function<void(const Event &)>;
    std::shared_ptr<Session> Create(const std::string &candidate, unsigned count,
                                    const std::string &resume_text = {},
                                    const std::string &owner = {}, const EventSink &sink = {});
    std::vector<Event> Answer(Session &session, const std::string &text, const EventSink &sink = {},
                              const std::string &frame = {});
    std::shared_ptr<Session> Find(const std::string &id);

  private:
    Model model_;
    std::string instance_id_;
    std::shared_ptr<ReportStore> reports_;
    std::mutex mutex_;
    std::unordered_map<std::string, std::shared_ptr<Session>> sessions_;
};
} // namespace backend
