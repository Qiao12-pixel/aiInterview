#include "infrastructure/storage/report_store.h"
#include "application/engine.h"
#include "infrastructure/llm.h"
#include <iostream>
#include <stdexcept>
int main(int argc, char **argv) {
    if (argc != 2) {
        std::cerr << "需要隔离的测试数据库配置文件\n";
        return 2;
    }
    auto store = backend::OpenReportStore(argv[1]);
    std::string id;
    {
        backend::Engine engine(backend::MakeMockModel(), store);
        auto session = engine.Create("持久化测试", 1);
        id = session->id;
        engine.Answer(*session, "RAII 管理资源");
        if (!session->completed)
            throw std::runtime_error("完成状态错误");
    }
    // 创建全新引擎和连接：模拟进程重启后的报告查询。
    backend::Engine restarted(backend::MakeMockModel(), backend::OpenReportStore(argv[1]));
    auto report = restarted.Report(id);
    if (!report || (*report)["candidate_name"] != "持久化测试" || (*report)["records"].size() != 1)
        throw std::runtime_error("重启读取失败");
    std::cout << "MySQL persistence and fresh-engine report retrieval passed\n";
}
