#include "transport/grpc_client.h"
namespace gateway {
void Configure(grpc::ClientContext &ctx, const std::string &token, unsigned seconds) {
    ctx.set_deadline(std::chrono::system_clock::now() + std::chrono::seconds(seconds));
    if (!token.empty())
        ctx.AddMetadata("authorization", "Bearer " + token);
}
void CheckRpc(const grpc::Status &s) {
    if (s.ok())
        return;
    unsigned code = 503;
    switch (s.error_code()) {
    case grpc::StatusCode::INVALID_ARGUMENT:
        code = 400;
        break;
    case grpc::StatusCode::UNAUTHENTICATED:
        code = 401;
        break;
    case grpc::StatusCode::PERMISSION_DENIED:
        code = 403;
        break;
    case grpc::StatusCode::NOT_FOUND:
        code = 404;
        break;
    case grpc::StatusCode::RESOURCE_EXHAUSTED:
    case grpc::StatusCode::FAILED_PRECONDITION:
        code = 409;
        break;
    default:
        break;
    }
    throw Error(code, s.error_message().empty() ? "后端 RPC 请求失败" : s.error_message());
}
Json SessionJson(const rpc::Session &s) {
    return {{"id", s.id()},
            {"candidate_name", s.candidate_name()},
            {"status", s.status()},
            {"question_index", s.question_index()},
            {"question_total", s.question_total()}};
}
Json EventJson(const rpc::InterviewEvent &e) {
    return {{"type", "event"},
            {"event_type", rpc::InterviewEvent::Type_Name(e.type())},
            {"session_id", e.session_id()},
            {"text", e.text()},
            {"json_payload", e.json_payload()}};
}
RpcClient::RpcClient(const std::string &address) {
    grpc::ChannelArguments args;
    args.SetMaxReceiveMessageSize(8 * 1024 * 1024);
    args.SetMaxSendMessageSize(8 * 1024 * 1024);
    stub_ = rpc::InterviewService::NewStub(
        grpc::CreateCustomChannel(address, grpc::InsecureChannelCredentials(), args));
}
Json RpcClient::Account(const std::string &operation, const Json &body, const std::string &token,
                        const std::string &source) {
    grpc::ClientContext ctx;
    Configure(ctx, token, 30);
    rpc::AccountRequest req;
    rpc::AccountResponse res;
    req.set_operation(operation);
    req.set_body_json(body.dump());
    req.set_source(source);
    const auto status = stub_->Account(&ctx, req, &res);
    if (!status.ok() && operation != "me" &&
        status.error_code() == grpc::StatusCode::RESOURCE_EXHAUSTED)
        throw Error(429, status.error_message());
    if (!status.ok() && status.error_code() == grpc::StatusCode::ALREADY_EXISTS)
        throw Error(409, status.error_message());
    CheckRpc(status);
    return Json::parse(res.result_json());
}
rpc::CreateSessionResponse RpcClient::Create(grpc::ClientContext &ctx,
                                             const rpc::CreateSessionRequest &req) {
    rpc::CreateSessionResponse result;
    CheckRpc(stub_->CreateSession(&ctx, req, &result));
    return result;
}
std::unique_ptr<grpc::ClientReader<rpc::InterviewEvent>>
RpcClient::Start(grpc::ClientContext &ctx, const rpc::CreateSessionRequest &req) {
    return stub_->StartInterview(&ctx, req);
}
std::unique_ptr<grpc::ClientReader<rpc::InterviewEvent>>
RpcClient::Answer(grpc::ClientContext &ctx, const rpc::TextMessage &req) {
    return stub_->SendText(&ctx, req);
}
Json RpcClient::Report(grpc::ClientContext &ctx, const std::string &id) {
    rpc::ReportRequest req;
    req.set_session_id(id);
    rpc::ReportResponse result;
    CheckRpc(stub_->GetReport(&ctx, req, &result));
    return Json::parse(result.report_json());
}
Json RpcClient::Reports(grpc::ClientContext &ctx, unsigned offset) {
    rpc::ListReportsRequest req;
    req.set_offset(offset);
    rpc::ListReportsResponse result;
    CheckRpc(stub_->ListReports(&ctx, req, &result));
    return Json::parse(result.reports_json());
}
} // namespace gateway
