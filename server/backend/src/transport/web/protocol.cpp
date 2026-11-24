#include "transport/web/protocol.h"
#include <stdexcept>
namespace backend::web {
std::string Frame(const std::string &data, unsigned char flag) {
    std::string out(5, '\0');
    out[0] = flag;
    auto n = data.size();
    for (int i = 4; i >= 1; --i) {
        out[i] = static_cast<char>(n & 255);
        n >>= 8;
    }
    return out + data;
}
std::string Unframe(const std::string &data) {
    if (data.size() < 5 || data[0] != 0)
        throw std::invalid_argument("Invalid gRPC-Web frame");
    size_t n = 0;
    for (int i = 1; i < 5; ++i)
        n = (n << 8) | static_cast<unsigned char>(data[i]);
    if (n != data.size() - 5)
        throw std::invalid_argument("Expected exactly one request frame");
    return data.substr(5);
}
std::string Trailers(const grpc::Status &status) {
    const char *hex = "0123456789ABCDEF";
    std::string message;
    for (unsigned char c : status.error_message()) {
        if (c >= 32 && c < 127 && c != '%')
            message += c;
        else {
            message += '%';
            message += hex[c >> 4];
            message += hex[c & 15];
        }
    }
    return Frame("grpc-status: " + std::to_string(status.error_code()) +
                     "\r\ngrpc-message: " + message + "\r\n",
                 128);
}
template <class Req, class Res, class Fn>
grpc::Status Unary(const std::string &data, const Emit &emit, Fn fn) {
    Req req;
    Res res;
    if (!req.ParseFromString(data))
        return {grpc::StatusCode::INVALID_ARGUMENT, "Invalid protobuf"};
    auto status = fn(&req, &res);
    if (status.ok() && !emit(res.SerializeAsString()))
        return {grpc::StatusCode::CANCELLED, "Disconnected"};
    return status;
}
template <class Req, class Fn>
grpc::Status Stream(const std::string &data, const Emit &emit, Fn fn) {
    Req req;
    if (!req.ParseFromString(data))
        return {grpc::StatusCode::INVALID_ARGUMENT, "Invalid protobuf"};
    api::Writer<aiinterview::v1::InterviewEvent> writer{
        [&](const auto &event) { return emit(event.SerializeAsString()); }};
    return fn(&req, &writer);
}
grpc::Status Dispatch(api::Service &service, api::Context &ctx, const std::string &method,
                      const std::string &data, const Emit &emit) {
    namespace rpc = aiinterview::v1;
#define UNARY(M, Q, R)                                                                             \
    if (method == #M)                                                                              \
    return Unary<rpc::Q, rpc::R>(data, emit,                                                       \
                                 [&](auto *q, auto *r) { return service.M(&ctx, q, r); })
    UNARY(Account, AccountRequest, AccountResponse);
    UNARY(CreateSession, CreateSessionRequest, CreateSessionResponse);
    UNARY(GetReport, ReportRequest, ReportResponse);
    UNARY(ListReports, ListReportsRequest, ListReportsResponse);
#undef UNARY
    if (method == "StartInterview")
        return Stream<rpc::CreateSessionRequest>(
            data, emit, [&](auto *q, auto *w) { return service.StartInterview(&ctx, q, w); });
    if (method == "SendText")
        return Stream<rpc::TextMessage>(
            data, emit, [&](auto *q, auto *w) { return service.SendText(&ctx, q, w); });
    return {grpc::StatusCode::UNIMPLEMENTED, "Unknown RPC"};
}
} // namespace backend::web
