#include "infrastructure/database/bootstrap.h"
#include <mysql.h>
#include <memory>
#include <stdexcept>
#include <string>
namespace backend::database {
namespace {
struct DeleteDb {
    void operator()(MYSQL *db) const {
        if (db)
            mysql_close(db);
    }
};
struct DeleteResult {
    void operator()(MYSQL_RES *result) const {
        if (result)
            mysql_free_result(result);
    }
};
void Query(MYSQL *db, const std::string &sql, const std::string &action) {
    if (mysql_real_query(db, sql.data(), sql.size()))
        throw std::runtime_error("MySQL 自动初始化失败：" + action + "（错误码 " +
                                 std::to_string(mysql_errno(db)) +
                                 "），请检查数据库账号 CREATE/ALTER/INDEX 权限");
}
bool Exists(MYSQL *db, const std::string &sql) {
    Query(db, sql, "检查表结构");
    std::unique_ptr<MYSQL_RES, DeleteResult> result(mysql_store_result(db));
    if (!result)
        throw std::runtime_error("MySQL 初始化结构查询失败");
    return mysql_num_rows(result.get()) > 0;
}
class Lock {
    MYSQL *db_;

  public:
    explicit Lock(MYSQL *db) : db_(db) {
        Query(db_, "SELECT GET_LOCK(CONCAT('aiinterview:init:',LEFT(SHA2(DATABASE(),256),40)),15)",
              "等待初始化锁");
        std::unique_ptr<MYSQL_RES, DeleteResult> result(mysql_store_result(db_));
        const auto row = result ? mysql_fetch_row(result.get()) : nullptr;
        if (!row || !row[0] || std::string(row[0]) != "1")
            throw std::runtime_error("MySQL 初始化锁超时，请稍后重试");
    }
    ~Lock() {
        if (!mysql_query(
                db_,
                "SELECT RELEASE_LOCK(CONCAT('aiinterview:init:',LEFT(SHA2(DATABASE(),256),40)))")) {
            std::unique_ptr<MYSQL_RES, DeleteResult> result(mysql_store_result(db_));
        }
    }
};
} // namespace
void Initialize(const nlohmann::json &config) {
    const auto name = config.at("database").get<std::string>();
    if (name.empty() || name.size() > 64 ||
        name.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_") !=
            std::string::npos)
        throw std::runtime_error("MySQL database 名称只允许 1–64 位字母、数字和下划线");
    std::unique_ptr<MYSQL, DeleteDb> db(mysql_init(nullptr));
    if (!db)
        throw std::runtime_error("MySQL 初始化失败");
    unsigned timeout = 20;
    mysql_options(db.get(), MYSQL_OPT_CONNECT_TIMEOUT, &timeout);
    mysql_options(db.get(), MYSQL_OPT_READ_TIMEOUT, &timeout);
    mysql_options(db.get(), MYSQL_OPT_WRITE_TIMEOUT, &timeout);
    const auto host = config.value("host", std::string("127.0.0.1")),
               user = config.at("user").get<std::string>(),
               password = config.value("password", std::string{}),
               socket = config.value("socket", std::string{});
    if (!mysql_real_connect(db.get(), host.c_str(), user.c_str(), password.c_str(), nullptr,
                            config.value("port", 3306), socket.empty() ? nullptr : socket.c_str(),
                            0))
        throw std::runtime_error("MySQL 自动初始化无法连接，请检查服务、账号和密码");
    if (mysql_select_db(db.get(), name.c_str())) {
        if (mysql_errno(db.get()) != 1049)
            throw std::runtime_error("MySQL 无权访问配置的数据库");
        Query(db.get(), "CREATE DATABASE IF NOT EXISTS `" + name + "` CHARACTER SET utf8mb4",
              "创建数据库");
        if (mysql_select_db(db.get(), name.c_str()))
            throw std::runtime_error("MySQL 无法选择已创建数据库");
    }
    Lock lock(db.get());
    Query(db.get(), R"SQL(CREATE TABLE IF NOT EXISTS users (
      id CHAR(32) CHARACTER SET ascii COLLATE ascii_bin PRIMARY KEY,
      email VARCHAR(254) CHARACTER SET ascii COLLATE ascii_bin NOT NULL UNIQUE,
      display_name VARCHAR(80) NOT NULL,
      password_hash VARCHAR(255) NOT NULL,
      created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP
    ) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4)SQL",
          "创建用户表");
    Query(db.get(), R"SQL(CREATE TABLE IF NOT EXISTS interview_reports (
      session_id VARCHAR(128) CHARACTER SET ascii COLLATE ascii_bin PRIMARY KEY,
      candidate_name VARCHAR(120) NOT NULL,
      report_json JSON NOT NULL,
      owner_user_id CHAR(32) CHARACTER SET ascii COLLATE ascii_bin NULL,
      created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
      updated_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
      INDEX idx_report_owner_created(owner_user_id,created_at,session_id)
    ) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4)SQL",
          "创建报告表");
    if (!Exists(db.get(), "SELECT 1 FROM information_schema.COLUMNS WHERE TABLE_SCHEMA=DATABASE() "
                          "AND TABLE_NAME='interview_reports' AND COLUMN_NAME='owner_user_id'"))
        Query(db.get(),
              "ALTER TABLE interview_reports ADD COLUMN owner_user_id CHAR(32) CHARACTER SET ascii "
              "COLLATE ascii_bin NULL",
              "添加报告归属字段");
    if (!Exists(db.get(),
                "SELECT 1 FROM information_schema.STATISTICS WHERE TABLE_SCHEMA=DATABASE() AND "
                "TABLE_NAME='interview_reports' AND INDEX_NAME='idx_report_owner_created'"))
        Query(db.get(),
              "CREATE INDEX idx_report_owner_created ON "
              "interview_reports(owner_user_id,created_at,session_id)",
              "创建报告归属索引");
}
} // namespace backend::database
