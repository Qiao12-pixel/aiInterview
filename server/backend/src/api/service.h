#pragma once
#include "interview.grpc.pb.h"
#include "application/engine.h"
#include "ports/identity.h"
#include "accounts/service.h"
#include <map>
#include <functional>
#include <optional>
#include "infrastructure/cluster/router.h"
namespace backend::api {
struct Context {
    std::map<std::string, std::string> metadata;
    std::string source;
    std::chrono::system_clock::time_point deadline = std::chrono::system_clock::time_point::max();
    std::function<bool()> cancelled = [] { return false; };
    const auto &client_metadata() const { return metadata; }
    std::string peer() const { return source; }
    bool IsCancelled() const { return cancelled(); }
};
template <class T> struct Writer {
    std::function<bool(const T &)> emit;
    bool Write(const T &value) { return emit(value); }
};
class Service final : public aiinterview::v1::InterviewService::Service {
  public:
    explicit Service(Model model, std::shared_ptr<ReportStore> reports = {},
                     IdentityResolver identity = {},
                     std::shared_ptr<accounts::Service> accounts = {}, std::string instanceId = {});
    Json ClusterStatus() const {
        if (!router_)
            return {{"cluster_enabled", false}, {"ready", true}};
        auto members = router_->Members();
        return {{"cluster_enabled", true},
                {"instance_id", router_->self()},
                {"members", members},
                {"ready", !members.empty()}};
    }
    void SetRouter(std::shared_ptr<cluster::Router> router) { router_ = std::move(router); }
    grpc::Status Account(grpc::ServerContext *, const aiinterview::v1::AccountRequest *,
                         aiinterview::v1::AccountResponse *) override;
    grpc::Status CreateSession(grpc::ServerContext *, const aiinterview::v1::CreateSessionRequest *,
                               aiinterview::v1::CreateSessionResponse *) override;
    grpc::Status StartInterview(grpc::ServerContext *,
                                const aiinterview::v1::CreateSessionRequest *,
                                grpc::ServerWriter<aiinterview::v1::InterviewEvent> *) override;
    grpc::Status SendText(grpc::ServerContext *, const aiinterview::v1::TextMessage *,
                          grpc::ServerWriter<aiinterview::v1::InterviewEvent> *) override;
    grpc::Status GetReport(grpc::ServerContext *, const aiinterview::v1::ReportRequest *,
                           aiinterview::v1::ReportResponse *) override;
    grpc::Status ListReports(grpc::ServerContext *, const aiinterview::v1::ListReportsRequest *,
                             aiinterview::v1::ListReportsResponse *) override;

    grpc::Status Account(Context *, const aiinterview::v1::AccountRequest *,
                         aiinterview::v1::AccountResponse *);
    grpc::Status CreateSession(Context *, const aiinterview::v1::CreateSessionRequest *,
                               aiinterview::v1::CreateSessionResponse *);
    grpc::Status StartInterview(Context *, const aiinterview::v1::CreateSessionRequest *,
                                Writer<aiinterview::v1::InterviewEvent> *);
    grpc::Status SendText(Context *, const aiinterview::v1::TextMessage *,
                          Writer<aiinterview::v1::InterviewEvent> *);
    grpc::Status GetReport(Context *, const aiinterview::v1::ReportRequest *,
                           aiinterview::v1::ReportResponse *);
    grpc::Status ListReports(Context *, const aiinterview::v1::ListReportsRequest *,
                             aiinterview::v1::ListReportsResponse *);

  private:
    Engine engine_;
    std::shared_ptr<cluster::Router> router_;
    IdentityResolver identity_;
    std::shared_ptr<accounts::Service> accounts_;
    std::string User(Context *context);
};
} // namespace backend::api
