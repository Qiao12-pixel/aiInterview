#include "transport/web/session.h"
#include "transport/web/protocol.h"
#include <filesystem>
#include <fstream>

namespace backend::web {
void Session::StaticFile(const http::request<http::string_body> &req, const std::string &target) {
    if (req.method() != http::verb::get) {
        Reply(404, "{}");
        return;
    }
    auto relative = target.substr(0, target.find('?'));
    if (relative == "/")
        relative = "/index.html";
    if (relative.find("..") != std::string::npos || relative.find('%') != std::string::npos ||
        relative.find('\\') != std::string::npos) {
        Reply(404, "{}");
        return;
    }
    auto base = std::filesystem::weakly_canonical(root);
    auto file = std::filesystem::weakly_canonical(base / relative.substr(1));
    auto rel = file.lexically_relative(base);
    if (rel.empty() || *rel.begin() == "..") {
        Reply(404, "{}");
        return;
    }
    std::ifstream input(file, std::ios::binary);
    if (!input) {
        Reply(404, "{}");
        return;
    }
    std::string data((std::istreambuf_iterator<char>(input)), {});
    auto ext = file.extension().string();
    Reply(200, data,
          ext == ".html"  ? "text/html; charset=utf-8"
          : ext == ".js"  ? "text/javascript; charset=utf-8"
          : ext == ".css" ? "text/css; charset=utf-8"
                          : "application/octet-stream");
}
} // namespace backend::web
