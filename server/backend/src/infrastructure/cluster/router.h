#pragma once
#include "discovery/zookeeper.h"
#include "interview.grpc.pb.h"
#include <functional>
#include <map>
#include <mutex>
#include <optional>
#include <chrono>
namespace backend::cluster {
struct Target {
    std::string id;
    std::shared_ptr<aiinterview::v1::InterviewService::Stub> stub;
};
// Selection only: the caller sends a side-effecting RPC at most once.
class Router {
    struct Peer {
        std::string address;
        std::shared_ptr<aiinterview::v1::InterviewService::Stub> stub;
        std::chrono::steady_clock::time_point retry{};
    };
    std::string self_;
    std::function<std::vector<discovery::Endpoint>()> snapshot_;
    std::mutex mutex_;
    std::map<std::string, Peer> peers_;
    uint64_t cursor_ = 0;

  public:
    Router(std::string self, std::function<std::vector<discovery::Endpoint>()> snapshot);
    std::vector<std::string> Members() const;
    Target Select(const std::string &session = {});
    void Observe(const Target &, const grpc::Status &);
    const std::string &self() const { return self_; }
};
} // namespace backend::cluster
