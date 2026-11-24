#include "transport/web/session.h"
#include "transport/web/protocol.h"

namespace backend::web {
void Session::NativeAccount(const http::request<http::string_body> &req, const std::string &target,
                            api::Context &ctx) {
    if (req.method() != http::verb::post) {
        Reply(405, "{}");
        return;
    }
    aiinterview::v1::AccountRequest q;
    aiinterview::v1::AccountResponse r;
    q.set_operation(target.find("native-login") != std::string::npos ? "login" : "register");
    q.set_body_json(req.body());
    q.set_source(ctx.source);
    auto status = service.Account(&ctx, &q, &r);
    if (!status.ok()) {
        Reply(status.error_code() == grpc::StatusCode::UNAUTHENTICATED ? 401 : 400,
              Json{{"message", status.error_message()}}.dump());
        return;
    }
    auto body = Json::parse(r.result_json());
    body["access_token"] = body.value("token", std::string{});
    body.erase("token");
    Reply(200, body.dump());
    return;
}
void Session::WebAccount(const std::string &payload, api::Context &ctx) {
    std::string cookie;
    grpc::Status status;
    aiinterview::v1::AccountRequest q;
    aiinterview::v1::AccountResponse r;
    if (!q.ParseFromString(payload)) {
        Reply(400, "{}");
        return;
    }
    q.set_source(ctx.source);
    status = service.Account(&ctx, &q, &r);
    if (status.ok()) {
        auto result = Json::parse(r.result_json());
        if (result.contains("token")) {
            cookie = "aiinterview_auth=" + result["token"].get<std::string>() +
                     "; HttpOnly; SameSite=Strict; Path=/; Max-Age=" +
                     std::to_string(result.value("ttl_seconds", 604800));
            result.erase("token");
        }
        if (q.operation() == "logout")
            cookie = "aiinterview_auth=; HttpOnly; SameSite=Strict; Path=/; Max-Age=0";
        r.set_result_json(result.dump());
    }
    Reply(200, (status.ok() ? Frame(r.SerializeAsString()) : std::string{}) + Trailers(status),
          "application/grpc-web+proto", cookie);
    return;
}
} // namespace backend::web
