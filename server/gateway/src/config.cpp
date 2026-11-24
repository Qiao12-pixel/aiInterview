#include "config.h"
#include <cstdlib>
#include <fstream>
#include <iostream>
namespace gateway {
namespace {
std::string Env(const char *name, const std::string &fallback) {
    auto p = std::getenv(name);
    return p && *p ? p : fallback;
}
} // namespace
Config LoadConfig(int argc, char **argv) {
    Config c;
    const std::filesystem::path root = GATEWAY_PROJECT_ROOT;
    c.file = Env("GATEWAY_CONFIG", "");
    c.web_root = Env("WEB_ROOT", (root / "web").string());
    c.host = Env("HOST", c.host);
    c.grpc_target = Env("GRPC_TARGET", c.grpc_target);
    c.origin = Env("ALLOWED_ORIGIN", "");
    c.secure_cookie = Env("COOKIE_SECURE", "") == "true";
    std::string port = Env("PORT", "8888");
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if (a == "--help") {
            std::cout << "--config path --host address --port number --grpc-target host:port "
                         "--web-root path --allowed-origin origin\n";
            std::exit(0);
        }
        if (i + 1 >= argc)
            throw Error(400, "启动参数缺少值");
        std::string v = argv[++i];
        if (a == "--config")
            c.file = v;
        else if (a == "--host")
            c.host = v;
        else if (a == "--port")
            port = v;
        else if (a == "--grpc-target")
            c.grpc_target = v;
        else if (a == "--web-root")
            c.web_root = v;
        else if (a == "--allowed-origin")
            c.origin = v;
        else
            throw Error(400, "未知启动参数");
    }
    if (port.empty() || port.find_first_not_of("0123456789") != std::string::npos)
        throw Error(400, "PORT 必须为 1–65535");
    const auto p = std::stoul(port);
    if (!p || p > 65535)
        throw Error(400, "PORT 必须为 1–65535");
    c.port = p;
    // 仅可选网关配置，不读取后端模型/数据库凭据。
    if (!c.file.empty()) {
        std::ifstream file(c.file);
        if (!file)
            throw Error(503, "无法读取网关配置文件");
        Json value;
        file >> value;
        const auto gateway = value.value("gateway", Json::object());
        // host/port 等启动参数由 CLI/env 控制；配置文件仅提供静态目录。
        if (gateway.contains("web_root"))
            c.web_root = gateway.at("web_root").get<std::string>();
    }
    c.web_root = std::filesystem::canonical(c.web_root);
    c.secure_cookie = c.secure_cookie || c.origin.rfind("https://", 0) == 0;
    return c;
}
} // namespace gateway
