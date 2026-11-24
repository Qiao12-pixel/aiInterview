#pragma once
#include "transport/http.h"
#include <thread>
#include <condition_variable>
namespace gateway {
class Sse {
    Stream &stream_;
    grpc::ClientContext &context_;
    std::mutex write_mutex_, wait_mutex_;
    std::condition_variable wake_;
    std::thread monitor_;
    bool stop_ = false, ended_ = false;
    void Chunk(const std::string &text);
    void Monitor();

  public:
    Sse(Stream &stream, const Request &req, grpc::ClientContext &context);
    ~Sse();
    void Send(const Json &event);
    void Finish(const std::string &error = {});
};
} // namespace gateway
