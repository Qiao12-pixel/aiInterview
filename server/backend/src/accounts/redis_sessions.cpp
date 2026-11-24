#include "accounts/repositories.h"
#include "accounts/crypto.h"
#include <hiredis/hiredis.h>
#include "infrastructure/storage/pools.h"
namespace backend::accounts {
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
using Context = std::unique_ptr<redisContext, ContextDelete>;
using Reply = std::unique_ptr<redisReply, ReplyDelete>;
auto Connect(const Json &config) { return storage::RedisPool(config)->Borrow(); }
void Check(const Reply &reply) {
    if (!reply || reply->type == REDIS_REPLY_ERROR)
        throw Error(503, "Redis 登录会话操作失败");
}
class Sessions final : public SessionRepository {
    Json config_;
    std::shared_ptr<storage::Pool<redisContext>> pool_;
    std::string prefix_;
    std::string Key(const std::string &token) const {
        return prefix_ + "auth:session:" + Digest(token);
    }

  public:
    explicit Sessions(Json c)
        : config_(std::move(c)), pool_(storage::RedisPool(config_)),
          prefix_(config_.value("prefix", std::string("aiinterview:report:"))) {
        auto redis = Connect(config_);
        Reply reply(static_cast<redisReply *>(redisCommand(redis.get(), "PING")));
        Check(reply);
    }
    void Save(const std::string &token, const Json &user, unsigned ttl) override {
        auto redis = Connect(config_);
        const auto key = Key(token), text = user.dump();
        Reply reply(
            static_cast<redisReply *>(redisCommand(redis.get(), "SET %b %b EX %u", key.data(),
                                                   key.size(), text.data(), text.size(), ttl)));
        Check(reply);
    }
    std::optional<Json> Find(const std::string &token) override {
        if (token.size() != 64 || token.find_first_not_of("0123456789abcdef") != std::string::npos)
            return {};
        auto redis = Connect(config_);
        const auto key = Key(token);
        Reply reply(
            static_cast<redisReply *>(redisCommand(redis.get(), "GET %b", key.data(), key.size())));
        Check(reply);
        if (reply->type == REDIS_REPLY_NIL)
            return {};
        if (reply->type != REDIS_REPLY_STRING)
            throw Error(503, "登录会话数据无效");
        return Json::parse(std::string(reply->str, reply->len));
    }
    void Delete(const std::string &token) override {
        if (token.empty())
            return;
        auto redis = Connect(config_);
        const auto key = Key(token);
        Reply reply(
            static_cast<redisReply *>(redisCommand(redis.get(), "DEL %b", key.data(), key.size())));
        Check(reply);
    }
    bool AllowAttempt(const std::string &source) override {
        auto redis = Connect(config_);
        const auto key = prefix_ + "limit:" + Digest(source);
        const char *script = "local n=redis.call('INCR',KEYS[1]);if n==1 then "
                             "redis.call('EXPIRE',KEYS[1],900) end;return n";
        Reply reply(static_cast<redisReply *>(
            redisCommand(redis.get(), "EVAL %s 1 %b", script, key.data(), key.size())));
        Check(reply);
        return reply->type == REDIS_REPLY_INTEGER && reply->integer <= 10;
    }
};
} // namespace
std::unique_ptr<SessionRepository> RedisSessions(const Json &config) {
    return std::make_unique<Sessions>(config);
}
} // namespace backend::accounts
