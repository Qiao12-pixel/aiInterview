#include "transport/http.h"
#include <fstream>
namespace gateway {
namespace {
std::string DecodePath(const std::string &input) {
    std::string out;
    for (size_t i = 0; i < input.size(); ++i) {
        char ch = input[i];
        if (ch == '%') {
            if (i + 2 >= input.size())
                throw Error(404, "Not found");
            auto digit = [](char c) {
                if (c >= '0' && c <= '9')
                    return c - '0';
                if (c >= 'a' && c <= 'f')
                    return c - 'a' + 10;
                if (c >= 'A' && c <= 'F')
                    return c - 'A' + 10;
                return -1;
            };
            const auto a = digit(input[i + 1]), b = digit(input[i + 2]);
            if (a < 0 || b < 0)
                throw Error(404, "Not found");
            ch = static_cast<char>((a << 4) | b);
            i += 2;
        }
        if (static_cast<unsigned char>(ch) < 32 || ch == '\\')
            throw Error(404, "Not found");
        out += ch;
    }
    return out;
}
} // namespace
void StaticFile(App &app, Stream &stream, const Request &req) {
    if (req.method() != http::verb::get && req.method() != http::verb::head)
        throw Error(405, "请使用 GET");
    auto url = DecodePath(Path(req));
    if (url == "/")
        url = "/index.html";
    if (url.empty() || url[0] != '/')
        throw Error(404, "Not found");
    std::error_code ec;
    auto file = std::filesystem::canonical(app.config.web_root / url.substr(1), ec);
    if (ec || !std::filesystem::is_regular_file(file))
        throw Error(404, "Not found");
    const auto relative = file.lexically_relative(app.config.web_root);
    if (relative.empty() || *relative.begin() == "..")
        throw Error(404, "Not found");
    const auto size = std::filesystem::file_size(file);
    if (size > 8 * 1024 * 1024)
        throw Error(413, "文件过大");
    const auto ext = file.extension().string();
    const std::string mime = ext == ".html"  ? "text/html; charset=utf-8"
                             : ext == ".js"  ? "text/javascript; charset=utf-8"
                             : ext == ".css" ? "text/css; charset=utf-8"
                                             : "application/octet-stream";
    http::response<http::string_body> response(http::status::ok, req.version());
    response.set(http::field::content_type, mime);
    response.set(http::field::cache_control, "no-store");
    response.set("X-Content-Type-Options", "nosniff");
    response.keep_alive(false);
    if (req.method() == http::verb::get) {
        std::ifstream input(file, std::ios::binary);
        if (!input)
            throw Error(404, "Not found");
        response.body().assign(std::istreambuf_iterator<char>(input), {});
        response.prepare_payload();
    } else
        response.content_length(size);
    http::write(stream, response);
}
} // namespace gateway
