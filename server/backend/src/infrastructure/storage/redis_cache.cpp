#include "infrastructure/storage/redis_cache.h"
#include <hiredis/hiredis.h>
#include <memory>
namespace backend::storage {
namespace {
struct RedisDelete {
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
std::optional<std::string> CacheReport(const nlohmann::json &config, const std::string &id,
                                       const std::string *value) {

    if (!config.value("enabled", true))
        return {};
    const auto host = config.value("host", std::string("127.0.0.1"));
    timeval timeout{1, 0};
    std::unique_ptr<redisContext, RedisDelete> ctx(
        redisConnectWithTimeout(host.c_str(), config.value("port", 6379), timeout));
    if (!ctx || ctx->err)
        return {};
    redisSetTimeout(ctx.get(), timeout);
    const auto password = config.value("password", std::string{});
    if (!password.empty()) {
        std::unique_ptr<redisReply, ReplyDelete> auth(static_cast<redisReply *>(
            redisCommand(ctx.get(), "AUTH %b", password.data(), password.size())));
        if (!auth || auth->type == REDIS_REPLY_ERROR)
            return {};
    }
    const auto prefix = config.value("prefix", std::string("aiinterview:report:")),
               key = prefix + id;
    std::unique_ptr<redisReply, ReplyDelete> reply(static_cast<redisReply *>(
        value ? redisCommand(ctx.get(), "SET %b %b EX %d", key.data(), key.size(), value->data(),
                             value->size(), config.value("ttl_seconds", 3600))
              : redisCommand(ctx.get(), "GET %b", key.data(), key.size())));
    if (!value && reply && reply->type == REDIS_REPLY_STRING)
        return std::string(reply->str, reply->len);
    return {};
}
} // namespace backend::storage
