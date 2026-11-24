#include "infrastructure/storage/report_store.h"
#include <mysql.h>
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
    std::mutex mutex_;
    std::unique_ptr<MYSQL, MysqlDelete> Connect() {
        std::unique_ptr<MYSQL, MysqlDelete> db(mysql_init(nullptr));
        if (!db)
            throw std::runtime_error("MySQL 初始化失败");
        unsigned timeout = 5;
        mysql_options(db.get(), MYSQL_OPT_CONNECT_TIMEOUT, &timeout);
        mysql_options(db.get(), MYSQL_OPT_READ_TIMEOUT, &timeout);
        mysql_options(db.get(), MYSQL_OPT_WRITE_TIMEOUT, &timeout);
        const auto host = mysql_.value("host", std::string("127.0.0.1"));
        const auto user = mysql_.at("user").get<std::string>(),
                   password = mysql_.value("password", std::string{}),
                   database = mysql_.at("database").get<std::string>();
        const auto socket = mysql_.value("socket", std::string{});
        if (!mysql_real_connect(db.get(), host.c_str(), user.c_str(), password.c_str(),
                                database.c_str(), mysql_.value("port", 3306),
                                socket.empty() ? nullptr : socket.c_str(), 0))
            throw std::runtime_error("MySQL 连接失败，请检查服务端 storage.mysql 配置");
        if (mysql_set_character_set(db.get(), "utf8mb4"))
            throw std::runtime_error("MySQL 编码设置失败");
        return db;
    }
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
    SqlRedisStore(Json mysql, Json redis) : mysql_(std::move(mysql)), redis_(std::move(redis)) {
        auto db = Connect();
        Query(db.get(),
              "SELECT session_id, report_json, owner_user_id FROM interview_reports LIMIT 0");
        if (redis_.value("ttl_seconds", 3600) < 1)
            throw std::runtime_error("Redis TTL 必须大于 0");
    }
    void Save(const std::string &id, const Json &report) override {
        const std::string serialized = report.dump();
        {
            std::lock_guard<std::mutex> lock(mutex_);
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
            Query(db.get(), query);
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
            std::lock_guard<std::mutex> lock(mutex_);
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
        std::lock_guard<std::mutex> lock(mutex_);
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
    return std::make_shared<SqlRedisStore>(config.at("mysql"),
                                           config.value("redis", Json::object()));
}
} // namespace backend
