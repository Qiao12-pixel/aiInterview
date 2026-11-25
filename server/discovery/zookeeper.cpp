#include "discovery/zookeeper.h"
#include <zookeeper/zookeeper.h>
#include <nlohmann/json.hpp>
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <stdexcept>
#include <algorithm>
namespace discovery {
class Registry::Impl {
  public:
    std::string hosts, root;
    Endpoint publisher;
    zhandle_t *handle = nullptr;
    std::atomic_int state{0};
    std::mutex mutex;
    std::condition_variable wake;
    bool stop = false;
    std::vector<Endpoint> endpoints;
    std::thread thread;
    static void Watch(zhandle_t *, int type, int state, const char *, void *ctx) {
        if (type == ZOO_SESSION_EVENT)
            static_cast<Impl *>(ctx)->state = state;
    }
    Impl(std::string h, std::string r, Endpoint p)
        : hosts(std::move(h)), root(std::move(r)), publisher(std::move(p)) {
        if (root.empty() || root[0] != '/' || root.back() == '/' ||
            root.find("//") != std::string::npos)
            throw std::runtime_error("ZooKeeper root 无效");
        if (!publisher.id.empty() &&
            (publisher.id.find_first_not_of(
                 "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-") !=
                 std::string::npos ||
             publisher.address.empty()))
            throw std::runtime_error(
                "实例 ID 只允许字母、数字和短横线，必须配置 advertised_address");
        zoo_set_debug_level(ZOO_LOG_LEVEL_ERROR);
        thread = std::thread([this] { Run(); });
    }
    ~Impl() {
        {
            std::lock_guard<std::mutex> lock(mutex);
            stop = true;
        }
        wake.notify_all();
        thread.join();
        if (handle)
            zookeeper_close(handle);
    }
    void Publish() {
        size_t offset = 1;
        while (true) {
            auto end = root.find('/', offset);
            const auto part = root.substr(0, end);
            const int code =
                zoo_create(handle, part.c_str(), "", 0, &ZOO_OPEN_ACL_UNSAFE, 0, nullptr, 0);
            if (code != ZOK && code != ZNODEEXISTS)
                return;
            if (end == std::string::npos)
                break;
            offset = end + 1;
        }
        if (publisher.id.empty())
            return;
        const auto path = root + "/" + publisher.id;
        const auto data =
            nlohmann::json{{"id", publisher.id}, {"address", publisher.address}}.dump();
        const auto code = zoo_create(handle, path.c_str(), data.data(), data.size(),
                                     &ZOO_OPEN_ACL_UNSAFE, ZOO_EPHEMERAL, nullptr, 0);
        // 已存在的 ID 绝不覆盖别的实例地址。
        if (code == ZNODEEXISTS) {
            Stat stat{};
            if (zoo_exists(handle, path.c_str(), 0, &stat) == ZOK &&
                stat.ephemeralOwner != zoo_client_id(handle)->client_id)
                state = ZOO_EXPIRED_SESSION_STATE;
        }
    }
    void Run() {
        while (true) {
            {
                std::lock_guard<std::mutex> lock(mutex);
                if (stop)
                    return;
            }
            if (!handle || state == ZOO_EXPIRED_SESSION_STATE) {
                if (handle)
                    zookeeper_close(handle);
                state = 0;
                handle = zookeeper_init(hosts.c_str(), Watch, 6000, nullptr, this, 0);
            }
            std::vector<Endpoint> next;
            if (handle && state == ZOO_CONNECTED_STATE) {
                Publish();
                String_vector children{};
                if (state == ZOO_CONNECTED_STATE &&
                    zoo_get_children(handle, root.c_str(), 0, &children) == ZOK) {
                    for (int i = 0; i < children.count; ++i) {
                        char buffer[4096];
                        int size = sizeof(buffer);
                        const auto path = root + "/" + children.data[i];
                        if (zoo_get(handle, path.c_str(), 0, buffer, &size, nullptr) != ZOK ||
                            size <= 0 || size >= static_cast<int>(sizeof(buffer)))
                            continue;
                        const auto json =
                            nlohmann::json::parse(std::string(buffer, size), nullptr, false);
                        Stat stat{};
                        if (zoo_exists(handle, path.c_str(), 0, &stat) != ZOK ||
                            !stat.ephemeralOwner)
                            continue;
                        if (json.is_object() && json.contains("id") && json["id"].is_string() &&
                            json.contains("address") && json["address"].is_string()) {
                            const auto id = json["id"].get<std::string>(),
                                       address = json["address"].get<std::string>();
                            if (id != children.data[i] || id.empty() || id.size() > 48 ||
                                id.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRS"
                                                     "TUVWXYZ0123456789-") != std::string::npos ||
                                address.empty() || address.size() > 256 ||
                                address.find(':') == std::string::npos)
                                continue;
                            next.push_back({id, address});
                        }
                    }
                    deallocate_String_vector(&children);
                }
            }
            {
                std::unique_lock<std::mutex> lock(mutex);
                endpoints = std::move(next);
                if (wake.wait_for(lock, std::chrono::seconds(1), [this] { return stop; }))
                    return;
            }
        }
    }
};
Registry::Registry(std::string hosts, std::string root, Endpoint publisher)
    : impl_(std::make_unique<Impl>(std::move(hosts), std::move(root), std::move(publisher))) {}
Registry::~Registry() = default;
std::vector<Endpoint> Registry::Snapshot() const {
    if (impl_->state != ZOO_CONNECTED_STATE)
        return {};
    std::lock_guard<std::mutex> lock(impl_->mutex);
    return impl_->endpoints;
}
} // namespace discovery
