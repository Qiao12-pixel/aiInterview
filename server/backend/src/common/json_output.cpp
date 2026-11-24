#include "common/json_output.h"
#include <stdexcept>
#include <cctype>
namespace backend::common {
nlohmann::json ParseOutput(const std::string &text) {
    // 先尝试完整 JSON，再扫描代码围栏/解释中括号平衡的对象；不截取最后一个花括号。
    auto full = nlohmann::json::parse(text, nullptr, false);
    if (full.is_object())
        return full;
    for (size_t start = 0; start < text.size(); ++start) {
        if (text[start] != '{')
            continue;
        int depth = 0;
        bool quoted = false, escaped = false;
        for (size_t i = start; i < text.size(); ++i) {
            char ch = text[i];
            if (quoted) {
                if (escaped)
                    escaped = false;
                else if (ch == '\\')
                    escaped = true;
                else if (ch == '"')
                    quoted = false;
                continue;
            }
            if (ch == '"')
                quoted = true;
            else if (ch == '{')
                ++depth;
            else if (ch == '}' && --depth == 0) {
                auto value =
                    nlohmann::json::parse(text.substr(start, i - start + 1), nullptr, false);
                if (value.is_object())
                    return value;
                break;
            }
        }
    }
    throw std::runtime_error("模型未返回有效 JSON，请重新提交或减少回答长度");
}
std::string VisibleField(const std::string &text, const std::string &field) {
    const auto key = nlohmann::json(field).dump();
    size_t start = text.find(key);
    if (start == std::string::npos)
        return {};
    start += key.size();
    while (start < text.size() && std::isspace(static_cast<unsigned char>(text[start])))
        ++start;
    if (start >= text.size() || text[start++] != ':')
        return {};
    while (start < text.size() && std::isspace(static_cast<unsigned char>(text[start])))
        ++start;
    if (start >= text.size() || text[start] != '"')
        return {};
    const size_t begin = start++;
    bool escaped = false;
    for (size_t i = start; i < text.size(); ++i) {
        const char ch = text[i];
        if (!escaped && ch == '"') {
            const auto value =
                nlohmann::json::parse(text.substr(begin, i - begin + 1), nullptr, false);
            return value.is_string() ? value.get<std::string>() : std::string{};
        }
        if (!escaped && ch == '\\')
            escaped = true;
        else
            escaped = false;
    }
    // 流可能停在 UTF-8、反斜杠或 Unicode 代理对中间，最多回退一个转义对。
    for (size_t trim = 0; trim <= 12 && begin + trim < text.size(); ++trim) {
        const auto value = nlohmann::json::parse(
            text.substr(begin, text.size() - begin - trim) + '"', nullptr, false);
        if (value.is_string())
            return value.get<std::string>();
    }
    return {};
}
} // namespace backend::common
