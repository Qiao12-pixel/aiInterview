#include "transport/http.h"
#include "common/codec.h"
namespace gateway {
void AccountRoute(App &app, Stream &stream, const Request &req) {
    const auto body = Body(req, 16 * 1024);
    const auto operation = Path(req).substr(std::string("/api/account/").size());
    const bool native = operation.rfind("native-", 0) == 0;
    const auto forwarded = native ? operation.substr(7) : operation;
    if (forwarded != "register" && forwarded != "login" && forwarded != "me" &&
        forwarded != "logout")
        throw Error(404, "账号接口不存在");
    const auto token = CookieToken(Header(req, http::field::cookie));
    boost::system::error_code ec;
    const auto peer = stream.socket().remote_endpoint(ec);
    const auto result =
        app.rpc.Account(forwarded, body, token, ec ? "local" : peer.address().to_string());
    if (!result.value("enabled", true))
        throw Error(503, "账号服务未配置");
    Json response;
    std::string cookie;
    const auto makeCookie = [&](const std::string &value, unsigned ttl) {
        return "aiinterview_auth=" + value +
               "; HttpOnly; SameSite=Strict; Path=/; Max-Age=" + std::to_string(ttl) +
               (app.config.secure_cookie ? "; Secure" : "");
    };
    if (forwarded == "register" || forwarded == "login") {
        response = {{"user", result.at("user")}};
        if (native)
            response["access_token"] = result.at("token");
        else
            cookie = makeCookie(result.at("token"), result.at("ttl_seconds").get<unsigned>());
    } else if (forwarded == "logout") {
        response = {{"ok", true}};
        cookie = makeCookie("", 0);
    } else
        response = {{"user", result.at("user")}};
    JsonResponse(stream, req, 200, response, cookie);
}
} // namespace gateway
