#include "transport/web/session.h"
#include "transport/web/protocol.h"
#include <sys/socket.h>

namespace backend::web {
namespace {
std::string Cookie(const std::string &all) {
    const std::string key = "aiinterview_auth=";
    size_t p = 0;
    while (p < all.size()) {
        while (p < all.size() && (all[p] == ' ' || all[p] == ';'))
            ++p;
        auto end = all.find(';', p);
        auto part = all.substr(p, end == std::string::npos ? end : end - p);
        if (part.rfind(key, 0) == 0)
            return part.substr(key.size());
        if (end == std::string::npos)
            break;
        p = end + 1;
    }
    return {};
}
} // namespace
void Session::Run() {
    stream.socket().set_option(tcp::no_delay(true));
    timeval timeout{10, 0};
    setsockopt(stream.socket().native_handle(), SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
    auto req = parser.release();
    std::string target(req.target());
    api::Context ctx;
    beast::error_code ep;
    auto endpoint = stream.socket().remote_endpoint(ep);
    ctx.source = ep ? "unknown" : endpoint.address().to_string();
    std::string token = Cookie(std::string(req[http::field::cookie]));
    auto auth = req[http::field::authorization];
    if (!auth.empty())
        ctx.metadata["authorization"] = std::string(auth);
    else if (!token.empty())
        ctx.metadata["authorization"] = "Bearer " + token;
    auto origin = req[http::field::origin];
    if (!origin.empty() && origin != "http://" + std::string(req[http::field::host]) &&
        origin != "https://" + std::string(req[http::field::host])) {
        Reply(403, "{}");
        return;
    }
    if (target == "/healthz") {
        auto health = service.ClusterStatus();
        health["ok"] = true;
        health["transport"] = "grpc-web";
        Reply(200, health.dump());
        return;
    }
    if (target == "/api/account/native-login" || target == "/api/account/native-register") {
        NativeAccount(req, target, ctx);
        return;
    }
    const std::string prefix = "/aiinterview.v1.InterviewService/";
    if (target.rfind(prefix, 0) == 0) {
        if (req.method() != http::verb::post ||
            req[http::field::content_type].find("application/grpc-web+proto") != 0) {
            Reply(415, "{}");
            return;
        }
        std::string payload;
        try {
            payload = Unframe(req.body());
        } catch (const std::exception &e) {
            Reply(400, Json{{"message", e.what()}}.dump());
            return;
        }
        const auto method = target.substr(prefix.size());
        Rpc(method, payload, ctx);
        return;
    }
    StaticFile(req, target);
}
} // namespace backend::web
