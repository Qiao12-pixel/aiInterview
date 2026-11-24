#include "interview/infrastructure/llm/http.h"
#include "interview/infrastructure/llm/stream.h"
#include <curl/curl.h>
#include <memory>
#include <stdexcept>
#include <exception>
namespace backend::llm {
namespace {
struct Runtime {
    Runtime() {
        if (curl_global_init(CURL_GLOBAL_DEFAULT))
            throw std::runtime_error("HTTP 初始化失败");
    }
    ~Runtime() { curl_global_cleanup(); }
};
struct Response {
    std::string raw;
    StreamDecoder decoder;
    bool streaming;
    std::exception_ptr error;
    Response(bool stream, TokenSink sink) : decoder(std::move(sink)), streaming(stream) {}
};
size_t Write(char *data, size_t size, size_t count, void *pointer) {
    auto &out = *static_cast<Response *>(pointer);
    const auto bytes = size * count;
    try {
        if (out.raw.size() + bytes > 2 * 1024 * 1024)
            throw std::runtime_error("模型响应过大");
        out.raw.append(data, bytes);
        if (out.streaming)
            out.decoder.Feed(std::string(data, bytes));
        return bytes;
    } catch (...) {
        out.error = std::current_exception();
        return 0;
    }
}
} // namespace
std::string Call(const Config &config, const nlohmann::json &body, const TokenSink &sink) {
    static Runtime runtime;
    std::unique_ptr<CURL, decltype(&curl_easy_cleanup)> curl(curl_easy_init(), curl_easy_cleanup);
    if (!curl)
        throw std::runtime_error("HTTP 初始化失败");
    curl_slist *raw = nullptr;
    for (const auto &header :
         {std::string("Content-Type: application/json"), "Authorization: Bearer " + config.key}) {
        auto *next = curl_slist_append(raw, header.c_str());
        if (!next) {
            curl_slist_free_all(raw);
            throw std::runtime_error("无法创建 HTTP 请求头");
        }
        raw = next;
    }
    std::unique_ptr<curl_slist, decltype(&curl_slist_free_all)> headers(raw, curl_slist_free_all);
    const auto serialized = body.dump();
    Response response(body.value("stream", false), sink);
    curl_easy_setopt(curl.get(), CURLOPT_URL, config.url.c_str());
    curl_easy_setopt(curl.get(), CURLOPT_HTTPHEADER, headers.get());
    curl_easy_setopt(curl.get(), CURLOPT_POSTFIELDS, serialized.c_str());
    curl_easy_setopt(curl.get(), CURLOPT_POSTFIELDSIZE, static_cast<long>(serialized.size()));
    curl_easy_setopt(curl.get(), CURLOPT_CONNECTTIMEOUT, 10L);
    curl_easy_setopt(curl.get(), CURLOPT_TIMEOUT, static_cast<long>(config.timeout));
    curl_easy_setopt(curl.get(), CURLOPT_WRITEFUNCTION, Write);
    curl_easy_setopt(curl.get(), CURLOPT_WRITEDATA, &response);
    const auto code = curl_easy_perform(curl.get());
    long status = 0;
    curl_easy_getinfo(curl.get(), CURLINFO_RESPONSE_CODE, &status);
    if (status && status != 200)
        throw std::runtime_error("模型服务返回 HTTP " + std::to_string(status));
    if (response.error)
        std::rethrow_exception(response.error);
    if (code != CURLE_OK)
        throw std::runtime_error("模型请求失败或超时，请检查连接");
    if (response.streaming) {
        if (!response.decoder.Complete())
            throw std::runtime_error("模型流意外中断");
        return response.decoder.Content();
    }
    const auto choice = nlohmann::json::parse(response.raw).at("choices").at(0);
    if (choice.value("finish_reason", std::string{}) == "length")
        throw std::runtime_error("模型输出被截断，请减少题目数量");
    return choice.at("message").at("content").get<std::string>();
}
} // namespace backend::llm
