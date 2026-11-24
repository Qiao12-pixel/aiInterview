#include "transport/web/server.h"
#include "transport/web/session.h"
#include <thread>
namespace backend::web {
struct Server::Impl {
    net::io_context io;
    tcp::acceptor acceptor;
    net::thread_pool workers;
    std::atomic<unsigned> pending{0};
    std::vector<std::thread> threads;
    api::Service &service;
    std::string root;
    unsigned limit;
    Impl(api::Service &s, const std::string &address, std::string r, unsigned n)
        : acceptor(io), workers(n), service(s), root(std::move(r)), limit(n * 8) {
        auto colon = address.rfind(':');
        if (colon == std::string::npos)
            throw std::invalid_argument("web address must be host:port");
        tcp::endpoint endpoint(net::ip::make_address(address.substr(0, colon)),
                               static_cast<unsigned short>(std::stoul(address.substr(colon + 1))));
        acceptor.open(endpoint.protocol());
        acceptor.set_option(net::socket_base::reuse_address(true));
        acceptor.bind(endpoint);
        acceptor.listen();
        Accept();
        for (int i = 0; i < 2; ++i)
            threads.emplace_back([this] { io.run(); });
    }
    void Accept() {
        acceptor.async_accept([this](auto ec, auto socket) {
            if (!ec)
                std::make_shared<Session>(std::move(socket), service, workers, pending, root, limit)
                    ->Start();
            if (acceptor.is_open())
                Accept();
        });
    }
    ~Impl() {
        io.stop();
        for (auto &t : threads)
            t.join();
        workers.join();
    }
};
Server::Server(api::Service &s, const std::string &address, const std::string &root,
               unsigned workers)
    : impl_(std::make_unique<Impl>(s, address, root, workers)) {}
Server::~Server() = default;
} // namespace backend::web
