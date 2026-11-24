#include "infrastructure/messaging/rabbitmq.h"
#include "infrastructure/messaging/report_document.h"
#include "infrastructure/database/bootstrap.h"
#include <mysql.h>
#include <fstream>
#include <iostream>
#include <csignal>
#include <thread>
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
        throw std::runtime_error("报告任务 MySQL 操作失败");
}
std::string Quote(MYSQL *db, const std::string &value) {
    std::string out(value.size() * 2 + 1, '\0');
    out.resize(mysql_real_escape_string(db, out.data(), value.data(), value.size()));
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
        throw std::runtime_error("报告任务无法连接 MySQL");
    mysql_set_character_set(db.get(), "utf8mb4");
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
        else
            return 2;
    }
    try {
        std::ifstream file(path);
        nlohmann::json config;
        file >> config;
        const auto mysql = config.at("storage").at("mysql"), mq = config.at("rabbitmq");
        if (!mq.value("enabled", false))
            throw std::runtime_error("请启用 rabbitmq");
        backend::database::Initialize(mysql);
        std::signal(SIGINT, Stop);
        std::signal(SIGTERM, Stop);
        std::signal(SIGPIPE, SIG_IGN);
        std::cout << "Async report task worker started" << std::endl;
        do {
            try {
                backend::messaging::Publisher consumer(mq);
                while (!stopping && consumer.ConsumeOnce([&](const std::string &messageId,
                                                             const std::string &payload) {
                    const auto event = nlohmann::json::parse(payload);
                    const auto id = event.at("session_id").get<std::string>(),
                               eventId = event.at("event_id").get<std::string>();
                    if (event.value("type", std::string{}) != "report.completed" ||
                        messageId != eventId)
                        throw std::runtime_error("报告事件格式无效");
                    auto db = Connect(mysql);
                    Query(db.get(), "SELECT report_json FROM interview_reports WHERE session_id=" +
                                        Quote(db.get(), id));
                    nlohmann::json report;
                    {
                        std::unique_ptr<MYSQL_RES, ResultDelete> result(
                            mysql_store_result(db.get()));
                        const auto row = result ? mysql_fetch_row(result.get()) : nullptr;
                        if (!row)
                            throw std::runtime_error("报告尚未就绪");
                        report = nlohmann::json::parse(
                            std::string(row[0], mysql_fetch_lengths(result.get())[0]));
                    }
                    const auto document = backend::messaging::RenderReportDocument(report);
                    Query(db.get(), "INSERT IGNORE INTO "
                                    "report_documents(event_id,session_id,document_text) VALUES(" +
                                        Quote(db.get(), eventId) + "," + Quote(db.get(), id) + "," +
                                        Quote(db.get(), document) + ")");
                    std::cout << "Prepared report document " << eventId << std::endl;
                })) {
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
