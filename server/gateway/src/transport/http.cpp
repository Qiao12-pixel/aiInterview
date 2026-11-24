#include "transport/http.h"
#include "common/codec.h"
namespace gateway {
std::string Header(const Request &req, http::field key) { return std::string(req[key]); }
std::string Path(const Request &req) {
    const std::string value(req.target());
    return value.substr(0, value.find('?'));
}
Json Body(const Request &req, size_t limit) {
    if (req.body().size() > limit)
        throw Error(413, "请求过大");
    auto value = Json::parse(req.body(), nullptr, false);
    if (!value.is_object())
        throw Error(400, "请求必须为 JSON 对象");
    return value;
}
void JsonResponse(Stream &stream, const Request &req, unsigned status, const Json &body,
                  const std::string &cookie) {
    http::response<http::string_body> result(static_cast<http::status>(status), req.version());
    result.set(http::field::content_type, "application/json; charset=utf-8");
    result.set(http::field::cache_control, "no-store");
    result.set("X-Content-Type-Options", "nosniff");
    if (!cookie.empty())
        result.set(http::field::set_cookie, cookie);
    result.keep_alive(false);
    result.body() = body.dump();
    result.prepare_payload();
    http::write(stream, result);
}
bool AllowedOrigin(const App &app, const Request &req) {
    const auto origin =
        app.config.origin.empty() ? "http://" + Header(req, http::field::host) : app.config.origin;
    return Header(req, http::field::origin) == origin;
}
std::string Authenticate(App &app, const Request &req) {
    const auto token = CookieToken(Header(req, http::field::cookie));
    app.rpc.Account("me", Json::object(), token);
    return token;
}
std::string AuthorizeInterview(App &app, const Request &req, const Json &body) {
    if (!body.contains("session_id") || !body["session_id"].is_string() ||
        body["session_id"].get_ref<const std::string &>().empty())
        throw Error(400, "请提供会话 ID");
    const auto authToken = CookieToken(Header(req, http::field::cookie));
    const auto state = app.rpc.Account("me", Json::object(), authToken);
    if (state.value("enabled", true))
        return authToken;
    const auto token = body.value("session_token", std::string{});
    if (!app.registry.Owns(token, body["session_id"]))
        throw Error(403, "会话已失效，请新建会话");
    return {};
}
void HandleHttp(App &app, Stream &stream, Request &req) {
    try {
        const auto path = Path(req);
        if (path == "/healthz") {
            JsonResponse(stream, req, 200, {{"status", "ok"}});
            return;
        }
        if (path.rfind("/api/", 0) != 0) {
            StaticFile(app, stream, req);
            return;
        }
        if (!AllowedOrigin(app, req))
            throw Error(403, "Origin 不匹配");
        if (req.method() != http::verb::post)
            throw Error(405, "请使用 POST");
        if (path.rfind("/api/account/", 0) == 0)
            AccountRoute(app, stream, req);
        else if (path == "/api/sessions")
            CreateInterview(app, stream, req);
        else if (path == "/api/answer")
            AnswerInterview(app, stream, req);
        else if (path == "/api/report")
            ReportRoute(app, stream, req);
        else if (path == "/api/reports")
            ReportsRoute(app, stream, req);
        else
            throw Error(404, "接口不存在");
    } catch (const Error &e) {
        JsonResponse(stream, req, e.status, {{"message", e.what()}});
    } catch (const Json::exception &) {
        JsonResponse(stream, req, 400, {{"message", "请求字段格式无效"}});
    } catch (const boost::system::system_error &) { /* 客户端已断开，不能再次写响应。 */
    } catch (const std::exception &) {
        JsonResponse(stream, req, 503, {{"message", "服务暂不可用"}});
    }
}
} // namespace gateway
