#include "infrastructure/messaging/rabbitmq.h"
#include "infrastructure/database/bootstrap.h"
#include <mysql.h>
#include <fstream>
#include <iostream>
#include <csignal>
#include <thread>
#include <unistd.h>
namespace {
volatile std::sig_atomic_t stopping = 0;
void Stop(int) { stopping = 1; }
struct DbDelete {
    void operator()(MYSQL *db) const {
        if (db)
            mysql_close(db);
    }
};
struct ResultDelete {
    void operator()(MYSQL_RES *result) const {
        if (result)
            mysql_free_result(result);
    }
};
void Query(MYSQL *db, const std::string &sql) {
    if (mysql_real_query(db, sql.data(), sql.size()))
        throw std::runtime_error("Outbox MySQL 操作失败");
}
std::string Quote(MYSQL *db, const std::string &value) {
    std::string out(value.size() * 2 + 1, '\0');
    const auto size = mysql_real_escape_string(db, out.data(), value.data(), value.size());
    out.resize(size);
    return "'" + out + "'";
}
std::unique_ptr<MYSQL, DbDelete> Connect(const nlohmann::json &c) {
    std::unique_ptr<MYSQL, DbDelete> db(mysql_init(nullptr));
    unsigned timeout = 5;
    mysql_options(db.get(), MYSQL_OPT_CONNECT_TIMEOUT, &timeout);
    mysql_options(db.get(), MYSQL_OPT_READ_TIMEOUT, &timeout);
    mysql_options(db.get(), MYSQL_OPT_WRITE_TIMEOUT, &timeout);
    if (!mysql_real_connect(
            db.get(), c.value("host", std::string("127.0.0.1")).c_str(),
            c.at("user").get<std::string>().c_str(), c.value("password", std::string{}).c_str(),
            c.at("database").get<std::string>().c_str(), c.value("port", 3306), nullptr, 0))
        throw std::runtime_error("Outbox 无法连接 MySQL");
    return db;
}
} // namespace
int main(int argc, char **argv) {
    std::string path = "server/config/default_config.json";
    bool once = false;
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if (a == "--config" && i + 1 < argc)
            path = argv[++i];
        else if (a == "--once")
            once = true;
        else {
            std::cerr << "--config path [--once]\n";
            return 2;
        }
    }
    try {
        std::ifstream file(path);
        nlohmann::json config;
        file >> config;
        const auto mysql = config.at("storage").at("mysql"), mq = config.at("rabbitmq");
        if (!mq.value("enabled", false))
            throw std::runtime_error("请启用 rabbitmq 配置");
        backend::database::Initialize(mysql);
        std::signal(SIGINT, Stop);
        std::signal(SIGTERM, Stop);
        std::signal(SIGPIPE, SIG_IGN);
        std::cout << "Report outbox worker started" << std::endl;
        do {
            try {
                auto db = Connect(mysql);
                backend::messaging::Publisher publisher(mq);
                while (!stopping) {
                    Query(db.get(), "START TRANSACTION");
                    try {
                        Query(
                            db.get(),
                            "SELECT event_id,payload_json FROM report_outbox WHERE published_at IS "
                            "NULL ORDER BY created_at,event_id LIMIT 1 FOR UPDATE SKIP LOCKED");
                        std::string id, payload;
                        {
                            std::unique_ptr<MYSQL_RES, ResultDelete> result(
                                mysql_store_result(db.get()));
                            if (!result)
                                throw std::runtime_error("Outbox 查询失败");
                            auto row = mysql_fetch_row(result.get());
                            if (row) {
                                const auto *lengths = mysql_fetch_lengths(result.get());
                                id.assign(row[0], lengths[0]);
                                payload.assign(row[1], lengths[1]);
                            }
                        }
                        if (id.empty()) {
                            Query(db.get(), "COMMIT");
                            break;
                        }
                        publisher.Publish(id, payload);
                        Query(db.get(), "UPDATE report_outbox SET published_at=CURRENT_TIMESTAMP "
                                        "WHERE event_id=" +
                                            Quote(db.get(), id));
                        Query(db.get(), "COMMIT");
                        std::cout << "Published report event " << id << std::endl;
                    } catch (...) {
                        mysql_rollback(db.get());
                        throw;
                    }
                }
            } catch (const std::exception &e) {
                std::cerr << e.what() << std::endl;
                if (once)
                    return 1;
            }
            if (once)
                break;
            for (int i = 0; i < 10 && !stopping; ++i)
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
        } while (!stopping);
    } catch (const std::exception &e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }
    return 0;
}
