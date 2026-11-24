#pragma once
#include "common/types.h"
#include <filesystem>
namespace gateway {
struct Config {
    std::string host = "127.0.0.1", grpc_target = "127.0.0.1:50051", origin;
    unsigned short port = 8888;
    bool secure_cookie = false;
    std::filesystem::path file, web_root;
};
Config LoadConfig(int argc, char **argv);
} // namespace gateway
