#pragma once
#include "api/service.h"
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <atomic>
namespace backend::web {
namespace net = boost::asio;
namespace beast = boost::beast;
namespace http = beast::http;
using tcp = net::ip::tcp;
struct Session : std::enable_shared_from_this<Session> {
    beast::tcp_stream stream;
    beast::flat_buffer buffer;
    http::request_parser<http::string_body> parser;
    api::Service &service;
    net::thread_pool &workers;
    std::atomic<unsigned> &pending;
    std::string root;
    unsigned limit;

    Session(tcp::socket, api::Service &, net::thread_pool &, std::atomic<unsigned> &, std::string,
            unsigned);
    void Start();
    void Close();
    void Busy();
    void Run();
    void Reply(unsigned, const std::string &, const std::string &content = "application/json",
               const std::string &cookie = {});
    void NativeAccount(const http::request<http::string_body> &, const std::string &,
                       api::Context &);
    void WebAccount(const std::string &, api::Context &);
    void Rpc(const std::string &, const std::string &, api::Context &);
    void StaticFile(const http::request<http::string_body> &, const std::string &);
};
} // namespace backend::web
