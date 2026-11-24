#include "infrastructure/storage/identity.h"
#include <nlohmann/json.hpp>
#include <hiredis/hiredis.h>
#include "infrastructure/storage/pools.h"
#include <openssl/sha.h>
#include <fstream>
#include <memory>
#include <sstream>
#include <iomanip>
#include <stdexcept>
namespace backend {
namespace {
struct ContextDelete {
    void operator()(redisContext *p) const {
        if (p)
            redisFree(p);
    }
};
struct ReplyDelete {
    void operator()(redisReply *p) const {
        if (p)
            freeReplyObject(p);
    }
};
} // namespace
IdentityResolver OpenIdentityResolver(const std::string &path) {
    std::ifstream file(path);
    nlohmann::json root;
    file >> root;
    const auto storage = root.value("storage", nlohmann::json::object());
    if (!storage.value("enabled", false))
        return {};
    const auto config = storage.value("redis", nlohmann::json::object());
    auto pool = storage::RedisPool(config);
    return [config, pool](const std::string &token) -> std::string {
        if (token.size() != 64 || token.find_first_not_of("0123456789abcdef") != std::string::npos)
            return {};
        unsigned char hash[SHA256_DIGEST_LENGTH];
        SHA256(reinterpret_cast<const unsigned char *>(token.data()), token.size(), hash);
        std::ostringstream digest;
        for (unsigned char ch : hash)
            digest << std::hex << std::setw(2) << std::setfill('0') << static_cast<unsigned>(ch);
        auto redis = pool->Borrow();
        const auto key = config.value("prefix", std::string("aiinterview:report:")) +
                         "auth:session:" + digest.str();
        std::unique_ptr<redisReply, ReplyDelete> reply(
            static_cast<redisReply *>(redisCommand(redis.get(), "GET %b", key.data(), key.size())));
        if (!reply)
            throw std::runtime_error("登录会话查询失败");
        if (reply->type == REDIS_REPLY_NIL)
            return {};
        if (reply->type != REDIS_REPLY_STRING)
            throw std::runtime_error("登录会话格式无效");
        const auto value = nlohmann::json::parse(std::string(reply->str, reply->len));
        return value.value("id", std::string{});
    };
}
} // namespace backend
