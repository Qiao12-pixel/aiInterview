#pragma once
#include "config.h"
#include "transport/grpc_client.h"
#include "transport/registry.h"
namespace gateway {
struct App {
    Config config;
    RpcClient rpc;
    Registry registry;
    explicit App(Config value) : config(std::move(value)), rpc(config.grpc_target) {}
};
} // namespace gateway
