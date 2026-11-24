#include "transport/web/session.h"
#include "transport/web/protocol.h"
#include <cstdlib>

namespace backend::web {
Session::Session(tcp::socket socket, api::Service &s, net::thread_pool &w, std::atomic<unsigned> &p,
                 std::string r, unsigned l)
    : stream(std::move(socket)), service(s), workers(w), pending(p), root(std::move(r)), limit(l) {
    parser.body_limit(6 * 1024 * 1024);
}
void Session::Start() {
    stream.expires_after(std::chrono::seconds(15));
    http::async_read(stream, buffer, parser, [self = shared_from_this()](auto ec, auto) {
        if (ec)
            return;
        if (self->pending.fetch_add(1) >= self->limit) {
            self->pending--;
            self->Busy();
            return;
        }
        net::post(self->workers, [self] {
            try {
                self->Run();
            } catch (...) {
            }
            self->Close();
            self->pending--;
        });
    });
}
void Session::Close() {
    beast::error_code ec;
    stream.socket().shutdown(tcp::socket::shutdown_both, ec);
    stream.socket().close(ec);
}
void Session::Reply(unsigned code, const std::string &body, const std::string &content,
                    const std::string &cookie) {
    http::response<http::string_body> res{static_cast<http::status>(code), 11};
    res.set(http::field::content_type, content);
    res.set(http::field::cache_control, "no-store");
    res.set("X-Content-Type-Options", "nosniff");
    if (!cookie.empty())
        res.set(http::field::set_cookie,
                cookie + (std::getenv("INTERVIEW_SECURE_COOKIE") ? "; Secure" : ""));
    res.keep_alive(false);
    res.body() = body;
    res.prepare_payload();
    http::write(stream.socket(), res);
}
void Session::Busy() {
    auto res =
        std::make_shared<http::response<http::string_body>>(http::status::service_unavailable, 11);
    res->set(http::field::content_type, "application/json");
    res->set(http::field::retry_after, "1");
    res->body() = "{\"message\":\"Backend busy; retry later\"}";
    res->prepare_payload();
    stream.expires_after(std::chrono::seconds(5));
    http::async_write(stream, *res,
                      [self = shared_from_this(), res](auto, auto) { self->Close(); });
}

} // namespace backend::web
