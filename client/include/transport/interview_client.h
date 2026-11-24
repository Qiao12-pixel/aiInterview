#pragma once
#include <memory>
#include <string>
#include <vector>
namespace interview::transport {
struct Event {
    std::string type, text, json;
};
struct CreatedSession {
    std::string id, question;
    unsigned total;
};
class InterviewClient {
  public:
    InterviewClient(const std::string &target, bool tls, const std::string &root_certificates = "");
    ~InterviewClient();
    CreatedSession Create(const std::string &candidate, unsigned count,
                          const std::string &resume_pdf);
    std::vector<Event> Answer(const std::string &id, const std::string &text);
    std::string Report(const std::string &id);
    // 关闭窗口时取消当前及尚未开始的 RPC。
    void Cancel();

  private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};
} // namespace interview::transport
