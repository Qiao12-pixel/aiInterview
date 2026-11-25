#include "infrastructure/cluster/router.h"
#include <grpcpp/grpcpp.h>
#include <algorithm>
#include <set>
#include <stdexcept>
namespace backend::cluster {
Router::Router(std::string self, std::function<std::vector<discovery::Endpoint>()> snapshot)
    : self_(std::move(self)), snapshot_(std::move(snapshot)) {}
std::vector<std::string> Router::Members() const {
    std::vector<std::string> ids;
    for (const auto &member : snapshot_())
        ids.push_back(member.id);
    std::sort(ids.begin(), ids.end());
    return ids;
}
Target Router::Select(const std::string &session) {
    auto endpoints = snapshot_();
    std::sort(endpoints.begin(), endpoints.end(),
              [](const auto &a, const auto &b) { return a.id < b.id; });
    std::string owner;
    if (!session.empty()) {
        auto split = session.find('_');
        if (split == std::string::npos || split == 0)
            throw std::invalid_argument("会话缺少实例标识");
        owner = session.substr(0, split);
    }
    std::lock_guard<std::mutex> lock(mutex_);
    std::set<std::string> live;
    for (const auto &e : endpoints)
        live.insert(e.id);
    for (auto it = peers_.begin(); it != peers_.end();)
        if (!live.count(it->first))
            it = peers_.erase(it);
        else
            ++it;
    std::vector<Target> candidates;
    const auto now = std::chrono::steady_clock::now();
    for (const auto &e : endpoints) {
        if (!owner.empty() && owner != e.id)
            continue;
        if (e.id == self_) {
            candidates.push_back({e.id, {}});
            continue;
        }
        auto &peer = peers_[e.id];
        if (peer.address != e.address) {
            peer.address = e.address;
            grpc::ChannelArguments args;
            args.SetInt("grpc.enable_retries", 0);
            args.SetMaxReceiveMessageSize(8 * 1024 * 1024);
            args.SetMaxSendMessageSize(6 * 1024 * 1024);
            peer.stub = aiinterview::v1::InterviewService::NewStub(
                grpc::CreateCustomChannel(e.address, grpc::InsecureChannelCredentials(), args));
            peer.retry = {};
        }
        if (peer.retry > now) {
            if (!owner.empty())
                throw std::runtime_error("会话所属实例暂不可用，请稍后确认状态");
            continue;
        }
        candidates.push_back({e.id, peer.stub});
    }
    if (candidates.empty())
        throw std::runtime_error(owner.empty() ? "没有可用实例，服务发现可能尚未就绪"
                                               : "会话所属实例已离线，未完成会话无法迁移");
    if (!owner.empty())
        return candidates.front();
    return candidates[cursor_++ % candidates.size()];
}
void Router::Observe(const Target &target, const grpc::Status &status) {
    if (!target.stub || status.error_code() != grpc::StatusCode::UNAVAILABLE)
        return;
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = peers_.find(target.id);
    if (it != peers_.end() && it->second.stub == target.stub)
        it->second.retry = std::chrono::steady_clock::now() + std::chrono::seconds(2);
}
} // namespace backend::cluster
