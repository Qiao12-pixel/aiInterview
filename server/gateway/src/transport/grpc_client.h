#pragma once
#include "common/types.h"
#include "interview.grpc.pb.h"
#include <grpcpp/grpcpp.h>
namespace gateway {
namespace rpc = aiinterview::v1;
void Configure(grpc::ClientContext &context, const std::string &token, unsigned seconds = 150);
void CheckRpc(const grpc::Status &status);
Json SessionJson(const rpc::Session &session);
Json EventJson(const rpc::InterviewEvent &event);
class RpcClient {
    std::unique_ptr<rpc::InterviewService::Stub> stub_;

  public:
    explicit RpcClient(const std::string &address);
    Json Account(const std::string &operation, const Json &body, const std::string &token,
                 const std::string &source = {});
    rpc::CreateSessionResponse Create(grpc::ClientContext &, const rpc::CreateSessionRequest &);
    std::unique_ptr<grpc::ClientReader<rpc::InterviewEvent>>
    Start(grpc::ClientContext &, const rpc::CreateSessionRequest &);
    std::unique_ptr<grpc::ClientReader<rpc::InterviewEvent>> Answer(grpc::ClientContext &,
                                                                    const rpc::TextMessage &);
    Json Report(grpc::ClientContext &, const std::string &id);
    Json Reports(grpc::ClientContext &, unsigned offset);
};
} // namespace gateway
