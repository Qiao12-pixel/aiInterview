#include "infrastructure/storage/report_store.h"
#include <mysql.h>
#include "infrastructure/storage/pools.h"
#include "infrastructure/database/bootstrap.h"
#include "infrastructure/storage/redis_cache.h"
#include <fstream>
#include <mutex>
#include <stdexcept>
#include <iostream>
#include <algorithm>
namespace backend {
namespace {
using Json = nlohmann::json;
struct MysqlDelete {
    void operator()(MYSQL *p) const {
        if (p)
            mysql_close(p);
    }
};
struct ResultDelete {
    void operator()(MYSQL_RES *p) const {
        if (p)
            mysql_free_result(p);
    }
};
class SqlRedisStore final : public ReportStore {
    Json mysql_, redis_;
    bool events_enabled_;
    std::shared_ptr<storage::Pool<MYSQL>> pool_;
    std::shared_ptr<storage::Pool<redisContext>> cache_pool_;
    storage::Pool<MYSQL>::Lease Connect() { return pool_->Borrow(); }
    std::string Quote(MYSQL *db, const std::string &text) {
        std::string escaped(text.size() * 2 + 1, '\0');
        const auto size = mysql_real_escape_string(db, escaped.data(), text.data(), text.size());
        escaped.resize(size);
        return "'" + escaped + "'";
    }
    void Query(MYSQL *db, const std::string &query) {
        if (mysql_real_query(db, query.data(), query.size()))
            throw std::runtime_error("MySQL 报告读写失败，请检查表结构与权限");
    }

  public:
    SqlRedisStore(Json mysql, Json redis, bool events)
        : mysql_(std::move(mysql)), redis_(std::move(redis)), events_enabled_(events),
          pool_(storage::MysqlPool(mysql_)), cache_pool_(storage::RedisPool(redis_)) {
        auto db = Connect();
        Query(db.get(),
              "SELECT session_id, report_json, owner_user_id FROM interview_reports LIMIT 0");
        std::unique_ptr<MYSQL_RES, ResultDelete> initial(mysql_store_result(db.get()));
        if (redis_.value("ttl_seconds", 3600) < 1)
            throw std::runtime_error("Redis TTL 必须大于 0");
    }
    void Save(const std::string &id, const Json &report) override {
        const std::string serialized = report.dump();
        {
            auto db = Connect();
            const auto query =
                "INSERT INTO "
                "interview_reports(session_id,candidate_name,report_json,owner_user_id) VALUES(" +
                Quote(db.get(), id) + "," +
                Quote(db.get(), report.value("candidate_name", std::string{})) + "," +
                Quote(db.get(), serialized) + "," +
                Quote(db.get(), report.value("owner_user_id", std::string{})) +
                ") ON DUPLICATE KEY UPDATE report_json=VALUES(report_json), "
                "candidate_name=VALUES(candidate_name), owner_user_id=VALUES(owner_user_id)";
            Query(db.get(), "START TRANSACTION");
            try {
                Query(db.get(), query);
                if (events_enabled_) {
                    const auto eventId = "report.completed:" + id;
                    const auto payload =
                        Json{{"event_id", eventId},
                             {"type", "report.completed"},
                             {"session_id", id},
                             {"owner_user_id", report.value("owner_user_id", std::string{})}}
                            .dump();
                    Query(db.get(),
                          "INSERT IGNORE INTO report_outbox(event_id,payload_json) VALUES(" +
                              Quote(db.get(), eventId) + "," + Quote(db.get(), payload) + ")");
                }
                Query(db.get(), "COMMIT");
            } catch (...) {
                mysql_rollback(db.get());
                throw;
            }
        }
        storage::CacheReport(redis_, id, &serialized); // 缓存不可用不影响已提交的 MySQL 数据。
    }
    std::optional<Json> Load(const std::string &id) override {
        auto cached = storage::CacheReport(redis_, id, nullptr);
        if (cached) {
            auto value = Json::parse(*cached, nullptr, false);
            if (value.is_object() && value.value("session_id", std::string{}) == id)
                return value;
        }
        std::string serialized;
        {
            auto db = Connect();
            Query(db.get(), "SELECT report_json FROM interview_reports WHERE session_id=" +
                                Quote(db.get(), id));
            std::unique_ptr<MYSQL_RES, ResultDelete> result(mysql_store_result(db.get()));
            if (!result)
                throw std::runtime_error("MySQL 查询结果无效");
            auto row = mysql_fetch_row(result.get());
            if (!row)
                return std::nullopt;
            serialized.assign(row[0], mysql_fetch_lengths(result.get())[0]);
        }
        auto report = Json::parse(serialized);
        storage::CacheReport(redis_, id, &serialized);
        return report;
    }
    Json List(const std::string &owner, unsigned offset) override {
        auto db = Connect();
        Query(db.get(), "SELECT session_id,candidate_name,created_at FROM interview_reports WHERE "
                        "owner_user_id=" +
                            Quote(db.get(), owner) +
                            " ORDER BY created_at DESC,session_id DESC LIMIT 20 OFFSET " +
                            std::to_string(std::min(offset, 10000u)));
        std::unique_ptr<MYSQL_RES, ResultDelete> result(mysql_store_result(db.get()));
        if (!result)
            throw std::runtime_error("MySQL 历史报告查询失败");
        Json rows = Json::array();
        MYSQL_ROW row;
        while ((row = mysql_fetch_row(result.get()))) {
            const auto *sizes = mysql_fetch_lengths(result.get());
            rows.push_back({{"session_id", std::string(row[0], sizes[0])},
                            {"candidate_name", std::string(row[1], sizes[1])},
                            {"created_at", std::string(row[2], sizes[2])}});
        }
        return rows;
    }
};
} // namespace
std::shared_ptr<ReportStore> OpenReportStore(const std::string &path) {
    std::ifstream file(path);
    if (!file)
        throw std::runtime_error("无法读取存储配置");
    Json root;
    file >> root;
    const auto config = root.value("storage", Json::object());
    if (!config.value("enabled", false))
        return {};
    database::Initialize(config.at("mysql"));
    return std::make_shared<SqlRedisStore>(
        config.at("mysql"), config.value("redis", Json::object()),
        root.value("rabbitmq", Json::object()).value("enabled", false));
}
} // namespace backend
