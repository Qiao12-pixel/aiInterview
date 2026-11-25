#include "infrastructure/cluster/router.h"
#include "api/routing.h"
#include "infrastructure/llm.h"
#include <iostream>
#define CHECK(x)                                                                                   \
    do {                                                                                           \
        if (!(x)) {                                                                                \
            std::cerr << "Failed: " << #x << "\n";                                                 \
            return 1;                                                                              \
        }                                                                                          \
    } while (0)
int main() {
    using backend::cluster::Router;
    std::vector<discovery::Endpoint> nodes{{"b", "127.0.0.1:2"}, {"a", "127.0.0.1:1"}};
    Router router("a", [&] { return nodes; });
    CHECK(router.Select().id == "a");
    auto remote = router.Select();
    CHECK(remote.id == "b" && remote.stub);
    CHECK(router.Select().id == "a");
    CHECK(router.Select("b_session").id == "b");
    CHECK(!router.Select("a_session").stub);
    router.Observe(remote, {grpc::StatusCode::UNAVAILABLE, "offline"});
    CHECK(router.Select().id == "a");
    try {
        router.Select("b_session");
        return 2;
    } catch (const std::runtime_error &) {
    }
    nodes = {{"a", "127.0.0.1:1"}};
    CHECK(router.Select().id == "a");
    try {
        router.Select("b_session");
        return 3;
    } catch (const std::runtime_error &) {
    }
    nodes.clear();
    try {
        router.Select();
        return 4;
    } catch (const std::runtime_error &) {
    }
    nodes = {{"a", "127.0.0.1:1"}, {"b", "127.0.0.1:3"}};
    CHECK(router.Select("b_session").stub);
    try {
        router.Select("invalid");
        return 5;
    } catch (const std::invalid_argument &) {
    }
    backend::api::Context ctx;
    ctx.metadata["x-aiinterview-routed"] = "1";
    aiinterview::v1::TextMessage req;
    req.set_session_id("b_session");
    backend::api::Writer<aiinterview::v1::InterviewEvent> out{[](const auto &) { return true; }};
    auto status = backend::api::RouteAnswer(&router, &ctx, &req, &out);
    CHECK(status && status->error_code() == grpc::StatusCode::FAILED_PRECONDITION);
    req.set_session_id("a_session");
    CHECK(!backend::api::RouteAnswer(&router, &ctx, &req, &out));
    std::cout << "Round robin, affinity, cooldown, discovery loss, membership changes and loop "
                 "prevention passed\n";
}
