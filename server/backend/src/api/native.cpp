#include "api/service.h"
namespace backend::api {
namespace rpc = aiinterview::v1;
namespace {
Context Adapt(grpc::ServerContext *source) {
    Context c;
    c.source = source->peer();
    c.cancelled = [source] { return source->IsCancelled(); };
    for (const auto &item : source->client_metadata())
        c.metadata.emplace(std::string(item.first.data(), item.first.size()),
                           std::string(item.second.data(), item.second.size()));
    return c;
}
} // namespace
grpc::Status Service::Account(grpc::ServerContext *c, const rpc::AccountRequest *req,
                              rpc::AccountResponse *out) {
    auto ctx = Adapt(c);
    return Account(&ctx, req, out);
}
grpc::Status Service::CreateSession(grpc::ServerContext *c, const rpc::CreateSessionRequest *req,
                                    rpc::CreateSessionResponse *out) {
    auto ctx = Adapt(c);
    return CreateSession(&ctx, req, out);
}
grpc::Status Service::StartInterview(grpc::ServerContext *c, const rpc::CreateSessionRequest *req,
                                     grpc::ServerWriter<rpc::InterviewEvent> *out) {
    auto ctx = Adapt(c);
    Writer<rpc::InterviewEvent> writer{
        [out](const rpc::InterviewEvent &e) { return out->Write(e); }};
    return StartInterview(&ctx, req, &writer);
}
grpc::Status Service::SendText(grpc::ServerContext *c, const rpc::TextMessage *req,
                               grpc::ServerWriter<rpc::InterviewEvent> *out) {
    auto ctx = Adapt(c);
    Writer<rpc::InterviewEvent> writer{
        [out](const rpc::InterviewEvent &e) { return out->Write(e); }};
    return SendText(&ctx, req, &writer);
}
grpc::Status Service::GetReport(grpc::ServerContext *c, const rpc::ReportRequest *req,
                                rpc::ReportResponse *out) {
    auto ctx = Adapt(c);
    return GetReport(&ctx, req, out);
}
grpc::Status Service::ListReports(grpc::ServerContext *c, const rpc::ListReportsRequest *req,
                                  rpc::ListReportsResponse *out) {
    auto ctx = Adapt(c);
    return ListReports(&ctx, req, out);
}
} // namespace backend::api
