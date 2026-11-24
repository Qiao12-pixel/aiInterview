#include "transport/web/session.h"
#include "transport/web/protocol.h"

namespace backend::web {
void Session::Rpc(const std::string &method, const std::string &payload, api::Context &ctx) {
    if (method == "Account") {
        WebAccount(payload, ctx);
        return;
    }
    http::response<http::empty_body> res{http::status::ok, 11};
    res.set(http::field::content_type, "application/grpc-web+proto");
    res.set(http::field::cache_control, "no-store");
    res.chunked(true);
    res.keep_alive(false);
    http::response_serializer<http::empty_body> serializer(res);
    http::write_header(stream.socket(), serializer);
    bool connected = true;
    ctx.cancelled = [&] { return !connected; };
    auto status = Dispatch(service, ctx, method, payload, [&](const std::string &bytes) {
        beast::error_code ec;
        const auto framed = Frame(bytes);
        net::write(stream.socket(), http::make_chunk(net::buffer(framed)), ec);
        connected = !ec;
        return connected;
    });
    if (connected) {
        const auto trailer = Trailers(status);
        net::write(stream.socket(), http::make_chunk(net::buffer(trailer)));
        net::write(stream.socket(), http::make_chunk_last());
    }
}
} // namespace backend::web
