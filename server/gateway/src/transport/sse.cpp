#include "transport/sse.h"
#include <poll.h>
#include <sys/socket.h>
namespace gateway {
Sse::Sse(Stream &stream, const Request &req, grpc::ClientContext &context)
    : stream_(stream), context_(context) {
    http::response<http::empty_body> header(http::status::ok, req.version());
    header.set(http::field::content_type, "text/event-stream; charset=utf-8");
    header.set(http::field::cache_control, "no-cache, no-transform");
    header.set("X-Accel-Buffering", "no");
    header.keep_alive(false);
    header.chunked(true);
    http::response_serializer<http::empty_body> serializer(header);
    http::write_header(stream_, serializer);
    monitor_ = std::thread([this] { Monitor(); });
}
Sse::~Sse() {
    {
        std::lock_guard<std::mutex> lock(wait_mutex_);
        stop_ = true;
    }
    wake_.notify_all();
    if (monitor_.joinable())
        monitor_.join();
}
void Sse::Chunk(const std::string &text) {
    net::write(stream_.socket(), http::make_chunk(net::buffer(text)));
}
void Sse::Send(const Json &event) {
    std::lock_guard<std::mutex> lock(write_mutex_);
    if (ended_)
        return;
    try {
        Chunk("data: " + event.dump() + "\n\n");
    } catch (...) {
        context_.TryCancel();
        throw;
    }
}
void Sse::Finish(const std::string &error) {
    std::lock_guard<std::mutex> lock(write_mutex_);
    if (ended_)
        return;
    ended_ = true;
    try {
        const Json event =
            error.empty() ? Json{{"type", "done"}} : Json{{"type", "error"}, {"message", error}};
        Chunk("data: " + event.dump() + "\n\n");
        net::write(stream_.socket(), http::make_chunk_last());
    } catch (...) {
        context_.TryCancel();
    }
}
void Sse::Monitor() {
    auto last = std::chrono::steady_clock::now();
    while (true) {
        {
            std::unique_lock<std::mutex> lock(wait_mutex_);
            if (wake_.wait_for(lock, std::chrono::milliseconds(200), [this] { return stop_; }))
                return;
        }
        pollfd fd{stream_.socket().native_handle(), POLLIN, 0};
        if (poll(&fd, 1, 0) > 0) {
            char ch;
            const auto count = recv(fd.fd, &ch, 1, MSG_PEEK | MSG_DONTWAIT);
            if (count == 0 || (fd.revents & (POLLHUP | POLLERR | POLLNVAL))) {
                context_.TryCancel();
                return;
            }
        }
        if (std::chrono::steady_clock::now() - last > std::chrono::seconds(15)) {
            try {
                std::lock_guard<std::mutex> lock(write_mutex_);
                if (ended_)
                    return;
                Chunk(": heartbeat\n\n");
                last = std::chrono::steady_clock::now();
            } catch (...) {
                context_.TryCancel();
                return;
            }
        }
    }
}
} // namespace gateway
