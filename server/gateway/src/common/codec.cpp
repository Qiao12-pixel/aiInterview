#include "common/codec.h"
#include <openssl/evp.h>
#include <cctype>
namespace gateway {
std::string Trim(std::string value) {
    const auto a = value.find_first_not_of(" \r\n\t"), b = value.find_last_not_of(" \r\n\t");
    return a == std::string::npos ? "" : value.substr(a, b - a + 1);
}
std::string EncodeBase64(const std::string &bytes) {
    if (bytes.empty())
        return {};
    std::string result(4 * ((bytes.size() + 2) / 3), '\0');
    EVP_EncodeBlock(reinterpret_cast<unsigned char *>(result.data()),
                    reinterpret_cast<const unsigned char *>(bytes.data()), bytes.size());
    return result;
}
std::string DecodeBase64(const std::string &value, size_t limit) {
    if (value.empty())
        return {};
    if (value.size() > 4 * ((limit + 2) / 3) || value.size() % 4)
        throw Error(400, "Base64 内容无效或文件过大");
    for (unsigned char ch : value)
        if (!std::isalnum(ch) && ch != '+' && ch != '/' && ch != '=')
            throw Error(400, "Base64 内容无效");
    std::string result(value.size() / 4 * 3, '\0');
    int count =
        EVP_DecodeBlock(reinterpret_cast<unsigned char *>(result.data()),
                        reinterpret_cast<const unsigned char *>(value.data()), value.size());
    if (count < 0)
        throw Error(400, "Base64 内容无效");
    if (value.back() == '=')
        --count;
    if (value.size() > 1 && value[value.size() - 2] == '=')
        --count;
    if (count < 0)
        throw Error(400, "Base64 内容无效");
    result.resize(count);
    if (result.size() > limit || EncodeBase64(result) != value)
        throw Error(400, "文件内容无效或超过大小限制");
    return result;
}
std::string Resume(const Json &body) {
    if (!body.contains("resume_pdf") || body["resume_pdf"] == "")
        return {};
    if (!body["resume_pdf"].is_string())
        throw Error(400, "简历必须是 PDF");
    const auto bytes = DecodeBase64(body["resume_pdf"], 5 * 1024 * 1024);
    if (bytes.compare(0, 5, "%PDF-") != 0)
        throw Error(400, "简历必须为有效 PDF，最大 5 MiB");
    return bytes;
}
std::string Frame(const Json &body) {
    if (!body.contains("video_jpeg") || body["video_jpeg"] == "")
        return {};
    if (!body["video_jpeg"].is_string())
        throw Error(400, "无效画面");
    const auto bytes = DecodeBase64(body["video_jpeg"], 512 * 1024);
    if (bytes.size() < 2 || static_cast<unsigned char>(bytes[0]) != 255 ||
        static_cast<unsigned char>(bytes[1]) != 216)
        throw Error(400, "画面必须为不超过 512 KiB 的 JPEG");
    return bytes;
}
void ValidateSession(const Json &body) {
    if (!body.is_object() || !body.contains("candidate_name") ||
        !body["candidate_name"].is_string() || Trim(body["candidate_name"]).empty() ||
        Trim(body["candidate_name"]).size() > 120 || !body.contains("question_total") ||
        !body["question_total"].is_number_integer())
        throw Error(400, "请输入姓名并选择 1–10 道题");
    const auto count = body["question_total"].get<int>();
    if (count < 1 || count > 10)
        throw Error(400, "题目数量须为 1–10");
}
std::string CookieToken(const std::string &header) {
    size_t i = 0;
    while (i < header.size()) {
        auto end = header.find(';', i);
        const auto part = Trim(header.substr(i, end == std::string::npos ? end : end - i));
        if (part.rfind("aiinterview_auth=", 0) == 0)
            return part.substr(17);
        if (end == std::string::npos)
            break;
        i = end + 1;
    }
    return {};
}
} // namespace gateway
