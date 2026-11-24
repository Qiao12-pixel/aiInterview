#pragma once
#include <memory>
#include <string>
#include <vector>
namespace discovery {
struct Endpoint {
    std::string id, address;
};
class Registry {
    class Impl;
    std::unique_ptr<Impl> impl_;

  public:
    // publisher 非空时注册临时节点；空时只订阅。连接丢失时清空发现快照。
    Registry(std::string hosts, std::string root, Endpoint publisher = {});
    ~Registry();
    std::vector<Endpoint> Snapshot() const;
};
} // namespace discovery
