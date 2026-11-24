#pragma once
#include "interview.grpc.pb.h"
#include "application/engine.h"
#include "ports/identity.h"
#include "accounts/service.h"
namespace backend::api {
class Service final : public aiinterview::v1::InterviewService::Service {
  public:
    explicit Service(Model model, std::shared_ptr<ReportStore> reports = {},
                     IdentityResolver identity = {},
                     std::shared_ptr<accounts::Service> accounts = {});
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

  private:
    Engine engine_;
    IdentityResolver identity_;
    std::shared_ptr<accounts::Service> accounts_;
    std::string User(grpc::ServerContext *context);
};
} // namespace backend::api
