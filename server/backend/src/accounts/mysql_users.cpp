#include "accounts/repositories.h"
#include <mysql.h>
#include <fstream>
#include <sstream>
namespace backend::accounts {
namespace {
struct DbDelete {
    void operator()(MYSQL *p) const {
        if (p)
            mysql_close(p);
    }
};
struct StmtDelete {
    void operator()(MYSQL_STMT *p) const {
        if (p)
            mysql_stmt_close(p);
    }
};
struct ResultDelete {
    void operator()(MYSQL_RES *p) const {
        if (p)
            mysql_free_result(p);
    }
};
using Db = std::unique_ptr<MYSQL, DbDelete>;
struct LibraryRuntime {
    LibraryRuntime() {
        if (mysql_library_init(0, nullptr, nullptr))
            throw Error(503, "MySQL 客户端初始化失败");
    }
    ~LibraryRuntime() { mysql_library_end(); }
};
struct ThreadRuntime {
    ThreadRuntime() {
        if (mysql_thread_init())
            throw Error(503, "MySQL 线程初始化失败");
    }
    ~ThreadRuntime() { mysql_thread_end(); }
};
Db Connect(const Json &c) {
    static LibraryRuntime library;
    thread_local ThreadRuntime thread;
    Db db(mysql_init(nullptr));
    if (!db)
        throw Error(503, "MySQL 初始化失败");
    unsigned timeout = 5;
    mysql_options(db.get(), MYSQL_OPT_CONNECT_TIMEOUT, &timeout);
    mysql_options(db.get(), MYSQL_OPT_READ_TIMEOUT, &timeout);
    mysql_options(db.get(), MYSQL_OPT_WRITE_TIMEOUT, &timeout);
    const auto host = c.value("host", std::string("127.0.0.1")),
               user = c.at("user").get<std::string>(),
               password = c.value("password", std::string{}),
               database = c.at("database").get<std::string>(),
               socket = c.value("socket", std::string{});
    if (!mysql_real_connect(db.get(), host.c_str(), user.c_str(), password.c_str(),
                            database.c_str(), c.value("port", 3306),
                            socket.empty() ? nullptr : socket.c_str(), 0))
        throw Error(503, "无法连接 MySQL，请检查配置");
    if (mysql_set_character_set(db.get(), "utf8mb4"))
        throw Error(503, "MySQL 编码设置失败");
    return db;
}
void Query(MYSQL *db, const std::string &sql) {
    if (mysql_real_query(db, sql.data(), sql.size()))
        throw Error(503, "MySQL 操作失败，请检查权限和数据库迁移");
}
std::unique_ptr<MYSQL_STMT, StmtDelete> Execute(MYSQL *db, const std::string &sql,
                                                std::vector<std::string> values) {
    std::unique_ptr<MYSQL_STMT, StmtDelete> stmt(mysql_stmt_init(db));
    if (!stmt || mysql_stmt_prepare(stmt.get(), sql.data(), sql.size()))
        throw Error(503, "MySQL SQL 准备失败");
    std::vector<MYSQL_BIND> parameters(values.size());
    std::vector<unsigned long> lengths(values.size());
    for (size_t i = 0; i < values.size(); ++i) {
        lengths[i] = values[i].size();
        parameters[i] = {};
        parameters[i].buffer_type = MYSQL_TYPE_STRING;
        parameters[i].buffer = values[i].data();
        parameters[i].buffer_length = lengths[i];
        parameters[i].length = &lengths[i];
    }
    if (!values.empty() && mysql_stmt_bind_param(stmt.get(), parameters.data()))
        throw Error(503, "MySQL 参数绑定失败");
    if (mysql_stmt_execute(stmt.get())) {
        if (mysql_stmt_errno(stmt.get()) == 1062)
            throw Error(409, "该邮箱已注册");
        throw Error(503, "MySQL 执行失败");
    }
    return stmt;
}
class Users final : public UserRepository {
    Json config_;

  public:
    explicit Users(Json config) : config_(std::move(config)) {
        auto db = Connect(config_);
        for (const auto *sql : {"SELECT id,email,password_hash FROM users LIMIT 0",
                                "SELECT owner_user_id FROM interview_reports LIMIT 0"}) {
            Query(db.get(), sql);
            std::unique_ptr<MYSQL_RES, ResultDelete> result(mysql_store_result(db.get()));
            if (!result)
                throw Error(503, "数据库结构检查失败");
        }
    }
    void Insert(const Json &user, const std::string &hash) override {
        auto db = Connect(config_);
        Execute(db.get(), "INSERT INTO users(id,email,display_name,password_hash) VALUES(?,?,?,?)",
                {user.at("id"), user.at("email"), user.at("display_name"), hash});
    }
    std::optional<Json> Find(const std::string &email) override {
        auto db = Connect(config_);
        auto stmt =
            Execute(db.get(), "SELECT id,email,display_name,password_hash FROM users WHERE email=?",
                    {email});
        std::array<std::array<char, 1024>, 4> buffers{};
        std::array<unsigned long, 4> lengths{};
        std::array<MYSQL_BIND, 4> columns{};
        for (size_t i = 0; i < 4; ++i) {
            columns[i] = {};
            columns[i].buffer_type = MYSQL_TYPE_STRING;
            columns[i].buffer = buffers[i].data();
            columns[i].buffer_length = buffers[i].size();
            columns[i].length = &lengths[i];
        }
        if (mysql_stmt_bind_result(stmt.get(), columns.data()) ||
            mysql_stmt_store_result(stmt.get()))
            throw Error(503, "用户查询失败");
        const auto status = mysql_stmt_fetch(stmt.get());
        if (status == MYSQL_NO_DATA)
            return {};
        if (status != 0)
            throw Error(503, "用户数据格式错误");
        Json value;
        const char *keys[] = {"id", "email", "display_name", "password_hash"};
        for (size_t i = 0; i < 4; ++i)
            value[keys[i]] = std::string(buffers[i].data(), lengths[i]);
        return value;
    }
};
} // namespace
std::unique_ptr<UserRepository> MysqlUsers(const Json &config) {
    return std::make_unique<Users>(config);
}
} // namespace backend::accounts
