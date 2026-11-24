#include "infrastructure/storage/pools.h"
#include <map>
namespace backend::storage {
namespace {
void MysqlThread() {
    static const bool initialized = [] {
        if (mysql_library_init(0, nullptr, nullptr))
            throw std::runtime_error("MySQL runtime init failed");
        return true;
    }();
    (void)initialized;
    struct Runtime {
        Runtime() {
            if (mysql_thread_init())
                throw std::runtime_error("MySQL 线程初始化失败");
        }
        ~Runtime() { mysql_thread_end(); }
    };
    thread_local Runtime runtime;
}
MYSQL *ConnectMysql(const nlohmann::json &c) {
    MysqlThread();
    MYSQL *db = mysql_init(nullptr);
    if (!db)
        throw std::runtime_error("MySQL 初始化失败");
    unsigned timeout = 5;
    mysql_options(db, MYSQL_OPT_CONNECT_TIMEOUT, &timeout);
    mysql_options(db, MYSQL_OPT_READ_TIMEOUT, &timeout);
    mysql_options(db, MYSQL_OPT_WRITE_TIMEOUT, &timeout);
    const auto host = c.value("host", std::string("127.0.0.1")),
               user = c.at("user").get<std::string>(),
               password = c.value("password", std::string{}),
               database = c.at("database").get<std::string>(),
               socket = c.value("socket", std::string{});
    if (!mysql_real_connect(db, host.c_str(), user.c_str(), password.c_str(), database.c_str(),
                            c.value("port", 3306), socket.empty() ? nullptr : socket.c_str(), 0) ||
        mysql_set_character_set(db, "utf8mb4")) {
        mysql_close(db);
        throw std::runtime_error("MySQL 连接失败，请检查配置");
    }
    return db;
}
redisContext *ConnectRedis(const nlohmann::json &c) {
    timeval timeout{1, 0};
    auto *redis = redisConnectWithTimeout(c.value("host", std::string("127.0.0.1")).c_str(),
                                          c.value("port", 6379), timeout);
    if (!redis || redis->err) {
        if (redis)
            redisFree(redis);
        throw std::runtime_error("Redis 连接失败");
    }
    redisSetTimeout(redis, timeout);
    const auto password = c.value("password", std::string{});
    if (!password.empty()) {
        auto *reply = static_cast<redisReply *>(
            redisCommand(redis, "AUTH %b", password.data(), password.size()));
        bool ok = reply && reply->type != REDIS_REPLY_ERROR;
        if (reply)
            freeReplyObject(reply);
        if (!ok) {
            redisFree(redis);
            throw std::runtime_error("Redis 认证失败");
        }
    }
    return redis;
}
} // namespace
std::shared_ptr<Pool<MYSQL>> MysqlPool(const nlohmann::json &config) {
    static std::mutex mutex;
    static std::map<std::string, std::weak_ptr<Pool<MYSQL>>> pools;
    std::lock_guard<std::mutex> lock(mutex);
    const auto key = config.dump();
    auto pool = pools[key].lock();
    if (!pool) {
        pool = std::make_shared<Pool<MYSQL>>(
            config.value("pool_size", 8u), [config] { return ConnectMysql(config); },
            [](MYSQL *db) {
                MysqlThread();
                mysql_close(db);
            },
            [](MYSQL *db) {
                MysqlThread();
                return mysql_ping(db) == 0 && mysql_rollback(db) == 0;
            });
        pools[key] = pool;
    }
    return pool;
}
std::shared_ptr<Pool<redisContext>> RedisPool(const nlohmann::json &config) {
    static std::mutex mutex;
    static std::map<std::string, std::weak_ptr<Pool<redisContext>>> pools;
    std::lock_guard<std::mutex> lock(mutex);
    const auto key = config.dump();
    auto pool = pools[key].lock();
    if (!pool) {
        pool = std::make_shared<Pool<redisContext>>(
            config.value("pool_size", 16u), [config] { return ConnectRedis(config); },
            [](redisContext *redis) { redisFree(redis); },
            [](redisContext *redis) { return redis->err == 0; });
        pools[key] = pool;
    }
    return pool;
}
} // namespace backend::storage
