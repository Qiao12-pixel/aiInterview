#include "transport/server.h"
#include "transport/http.h"
#include "transport/websocket.h"
#include <boost/beast/websocket.hpp>
#include <csignal>
#include <atomic>
#include <iostream>
#include <thread>
#include <vector>
#include <sys/socket.h>
#include <unistd.h>
namespace gateway {
namespace {
volatile std::sig_atomic_t stopping = 0;
void Signal(int) { stopping = 1; }
struct Worker {
    std::thread thread;
    std::shared_ptr<std::atomic_bool> done;
    int control;
};
void Connection(App &app, std::shared_ptr<net::ip::tcp::socket> socket) {
    Stream stream(std::move(*socket));
    beast::flat_buffer buffer;
    http::request_parser<http::string_body> parser;
    parser.body_limit(7 * 1024 * 1024);
    parser.header_limit(16 * 1024);
    try {
        http::read(stream, buffer, parser);
        auto req = parser.release();
        if (beast::websocket::is_upgrade(req))
            HandleWebSocket(app, std::move(stream), std::move(req));
        else
            HandleHttp(app, stream, req);
    } catch (const boost::system::system_error &e) {
        if (e.code() == http::error::body_limit) {
            Request req;
            JsonResponse(stream, req, 413, {{"message", "请求过大"}});
        }
    } catch (const std::exception &) {
    }
    boost::system::error_code ec;
    if (stream.socket().is_open())
        stream.socket().shutdown(net::ip::tcp::socket::shutdown_both, ec);
}
} // namespace
void Run(App &app) {
    std::signal(SIGINT, Signal);
    std::signal(SIGTERM, Signal);
    std::signal(SIGPIPE, SIG_IGN);
    net::io_context io;
    net::ip::tcp::acceptor acceptor(io);
    const auto address = net::ip::make_address(app.config.host);
    net::ip::tcp::endpoint endpoint(address, app.config.port);
    acceptor.open(endpoint.protocol());
    acceptor.set_option(net::socket_base::reuse_address(true));
    acceptor.bind(endpoint);
    acceptor.listen();
    acceptor.non_blocking(true);
    std::cout << "Web/WS gateway: http://"
              << (address.is_v6() ? "[" + app.config.host + "]" : app.config.host) << ":"
              << app.config.port << "\ngRPC upstream: " << app.config.grpc_target << std::endl;
    std::vector<Worker> workers;
    while (!stopping) {
        for (auto it = workers.begin(); it != workers.end();)
            if (it->done->load()) {
                it->thread.join();
                ::close(it->control);
                it = workers.erase(it);
            } else
                ++it;
        auto socket = std::make_shared<net::ip::tcp::socket>(io);
        boost::system::error_code ec;
        acceptor.accept(*socket, ec);
        if (ec == net::error::would_block || ec == net::error::try_again) {
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
            continue;
        }
        if (ec)
            throw boost::system::system_error(ec);
        if (workers.size() >= 128) {
            socket->close();
            continue;
        }
        // 同步读写使用系统超时，慢连接不能无限占用工作线程。
        timeval timeout{180, 0};
        setsockopt(socket->native_handle(), SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
        setsockopt(socket->native_handle(), SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
        const int control = ::dup(socket->native_handle());
        if (control < 0) {
            socket->close();
            continue;
        }
        auto done = std::make_shared<std::atomic_bool>(false);
        workers.push_back({std::thread([&, socket, done] {
                               try {
                                   Connection(app, socket);
                               } catch (...) {
                               }
                               done->store(true);
                           }),
                           done, control});
    }
    for (auto &worker : workers)
        ::shutdown(worker.control, SHUT_RDWR);
    for (auto &worker : workers) {
        if (worker.thread.joinable())
            worker.thread.join();
        ::close(worker.control);
    }
}
} // namespace gateway
