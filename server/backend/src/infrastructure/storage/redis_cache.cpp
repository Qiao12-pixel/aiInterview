#include "infrastructure/storage/redis_cache.h"
#include <hiredis/hiredis.h>
#include "infrastructure/storage/pools.h"
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
    std::optional<Pool<redisContext>::Lease> connection;
    try {
        connection.emplace(RedisPool(config)->Borrow());
    } catch (...) {
        return {};
    }
    auto &ctx = *connection;
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
