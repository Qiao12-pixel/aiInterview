#pragma once
#include "interview/domain/session.h"
#include "interview/ports/model.h"
#include <mutex>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
namespace backend {
class Engine {
  public:
    explicit Engine(Model model) : model_(std::move(model)) {}
    std::shared_ptr<Session> Create(const std::string &candidate, unsigned count,
                                    const std::string &resume_text = {});
    using EventSink = std::function<void(const Event &)>;
    std::vector<Event> Answer(Session &session, const std::string &text, const EventSink &sink = {},
                              const std::string &frame = {});
    std::shared_ptr<Session> Find(const std::string &id);

  private:
    Model model_;
    std::mutex mutex_;
    std::unordered_map<std::string, std::shared_ptr<Session>> sessions_;
};
} // namespace backend
