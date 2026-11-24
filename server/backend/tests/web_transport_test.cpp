#include "transport/web/server.h"
#include "transport/web/protocol.h"
#include "infrastructure/llm.h"
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <future>
#include <thread>
#include <iostream>
using namespace std::chrono_literals;
namespace net = boost::asio;
namespace http = boost::beast::http;
using tcp = net::ip::tcp;
int main() {
    net::io_context io;
    tcp::acceptor probe(io, {tcp::v4(), 0});
    auto port = probe.local_endpoint().port();
    probe.close();
    std::promise<void> release, entered;
    auto gate = release.get_future().share();
    std::atomic<bool> notified{false};
    std::atomic<unsigned> sent{0};
    auto mock = backend::MakeMockModel();
    backend::api::Service service([&](const auto &op, const auto &data, const auto &sink) {
        if (op == "questions") {
            if (!notified.exchange(true))
                entered.set_value();
            gate.wait();
        }
        return mock(op, data, sink);
    });
    backend::web::Server server(service, "127.0.0.1:" + std::to_string(port), "web", 1);
    std::vector<std::unique_ptr<tcp::socket>> idle;
    for (int i = 0; i < 128; ++i) {
        auto socket = std::make_unique<tcp::socket>(io);
        socket->connect({net::ip::make_address("127.0.0.1"), port});
        net::write(*socket, net::buffer("GET /healthz HTTP/1.1\r\n", 23));
        idle.push_back(std::move(socket));
    }
    auto request = [&](bool interview) {
        net::io_context ctx;
        tcp::socket socket(ctx);
        socket.connect({net::ip::make_address("127.0.0.1"), port});
        http::request<http::string_body> req{
            interview ? http::verb::post : http::verb::get,
            interview ? "/aiinterview.v1.InterviewService/StartInterview" : "/healthz", 11};
        req.set(http::field::host, "127.0.0.1");
        if (interview) {
            aiinterview::v1::CreateSessionRequest payload;
            payload.set_candidate_name("Synthetic transport test");
            payload.set_question_total(1);
            req.set(http::field::content_type, "application/grpc-web+proto");
            req.body() = backend::web::Frame(payload.SerializeAsString());
        }
        req.prepare_payload();
        http::write(socket, req);
        if (interview)
            ++sent;
        boost::beast::flat_buffer buffer;
        http::response<http::string_body> response;
        http::read(socket, buffer, response);
        return response;
    };
    if (request(false).result_int() != 200)
        return 1; // 128 idle connections don't occupy the single worker.
    std::vector<std::future<http::response<http::string_body>>> requests;
    for (int i = 0; i < 8; ++i)
        requests.push_back(std::async(std::launch::async, [&] { return request(true); }));
    entered.get_future().wait();
    while (sent.load() < 8)
        std::this_thread::yield();
    std::this_thread::sleep_for(100ms);
    auto excess = std::async(std::launch::async, [&] { return request(false); });
    bool available = excess.wait_for(200ms) == std::future_status::ready;
    release.set_value();
    auto rejected = excess.get();
    for (auto &pending : requests) {
        if (pending.get().result_int() != 200)
            return 2;
    }
    if (!available || rejected.result_int() != 503 || rejected[http::field::retry_after] != "1")
        return 3;
    if (request(false).result_int() != 200)
        return 4;
    std::cout << "128 idle HTTP connections, bounded queue 503/Retry-After, and recovery passed\n";
}
