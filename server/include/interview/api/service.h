#pragma once
#include "interview.grpc.pb.h"
#include "interview/engine.h"
namespace backend::api {
class Service final : public aiinterview::v1::InterviewService::Service {
  public:
    explicit Service(Model model);
    grpc::Status CreateSession(grpc::ServerContext *, const aiinterview::v1::CreateSessionRequest *,
                               aiinterview::v1::CreateSessionResponse *) override;
    grpc::Status SendText(grpc::ServerContext *, const aiinterview::v1::TextMessage *,
                          grpc::ServerWriter<aiinterview::v1::InterviewEvent> *) override;
    grpc::Status GetReport(grpc::ServerContext *, const aiinterview::v1::ReportRequest *,
                           aiinterview::v1::ReportResponse *) override;

  private:
    Engine engine_;
};
} // namespace backend::api
