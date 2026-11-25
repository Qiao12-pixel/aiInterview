#include "api/routing.h"
#include <thread>
#include <condition_variable>
namespace backend::api {
namespace rpc = aiinterview::v1;
namespace {
class Call {
    std::mutex mutex_;
    std::condition_variable wake_;
    bool done_ = false;
    std::thread watch_;

  public:
    grpc::ClientContext remote;
    explicit Call(Context *source) {
        remote.set_deadline(
            std::min(source->deadline, std::chrono::system_clock::now() + std::chrono::minutes(5)));
        auto it = source->metadata.find("authorization");
        if (it != source->metadata.end())
            remote.AddMetadata("authorization", it->second);
        remote.AddMetadata("x-aiinterview-routed", "1");
        watch_ = std::thread([this, source] {
            std::unique_lock<std::mutex> lock(mutex_);
            while (!wake_.wait_for(lock, std::chrono::milliseconds(50), [&] { return done_; })) {
                if (source->IsCancelled()) {
                    remote.TryCancel();
                    return;
                }
            }
        });
    }
    ~Call() {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            done_ = true;
        }
        wake_.notify_one();
        watch_.join();
    }
};
template <class Fn>
std::optional<grpc::Status> Route(cluster::Router *router, Context *ctx, const std::string &owner,
                                  Fn invoke) {
    if (!router)
        return {};
    if (ctx->metadata.count("x-aiinterview-routed")) {
        // A routed call is terminal. Never recursively forward even if the registry changes.
        if (!owner.empty() && owner.rfind(router->self() + "_", 0) != 0)
            return grpc::Status{grpc::StatusCode::FAILED_PRECONDITION, "路由目标与会话归属不匹配"};
        return {};
    }
    try {
        auto target = router->Select(owner);
        if (!target.stub)
            return {};
        Call call(ctx);
        const auto status = invoke(*target.stub, call.remote);
        router->Observe(target, status);
        return status;
    } catch (const std::invalid_argument &e) {
        return grpc::Status{grpc::StatusCode::INVALID_ARGUMENT, e.what()};
    } catch (const std::exception &e) {
        return grpc::Status{grpc::StatusCode::UNAVAILABLE, e.what()};
    }
}
template <class Reader>
grpc::Status Forward(Reader reader, grpc::ClientContext &ctx, Writer<rpc::InterviewEvent> *out) {
    rpc::InterviewEvent event;
    try {
        while (reader->Read(&event)) {
            if (!out->Write(event)) {
                ctx.TryCancel();
                break;
            }
        }
    } catch (...) {
        ctx.TryCancel();
        throw;
    }
    return reader->Finish();
}
} // namespace
std::optional<grpc::Status> RouteCreate(cluster::Router *r, Context *c,
                                        const rpc::CreateSessionRequest *q,
                                        rpc::CreateSessionResponse *out) {
    return Route(r, c, {},
                 [&](auto &stub, auto &ctx) { return stub.CreateSession(&ctx, *q, out); });
}
std::optional<grpc::Status> RouteStart(cluster::Router *r, Context *c,
                                       const rpc::CreateSessionRequest *q,
                                       Writer<rpc::InterviewEvent> *out) {
    return Route(r, c, {}, [&](auto &stub, auto &ctx) {
        return Forward(stub.StartInterview(&ctx, *q), ctx, out);
    });
}
std::optional<grpc::Status> RouteAnswer(cluster::Router *r, Context *c, const rpc::TextMessage *q,
                                        Writer<rpc::InterviewEvent> *out) {
    return Route(r, c, q->session_id(),
                 [&](auto &stub, auto &ctx) { return Forward(stub.SendText(&ctx, *q), ctx, out); });
}
} // namespace backend::api
