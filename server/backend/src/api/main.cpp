#include "api/service.h"
#include "transport/web/server.h"
#include "infrastructure/llm.h"
#include <grpcpp/grpcpp.h>
#include <iostream>
#include "infrastructure/storage/report_store.h"
#include "infrastructure/storage/identity.h"
#include <fstream>
#include "discovery/zookeeper.h"
int main(int argc, char **argv) {
    std::string config = "server/config/default_config.json", address = "127.0.0.1:50051";
    std::string web_address = "127.0.0.1:8888", web_root = "web";
    bool mock = false, mock_model = false;
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--web-listen" && i + 1 < argc)
            web_address = argv[++i];
        else if (arg == "--web-root" && i + 1 < argc)
            web_root = argv[++i];
        else if (arg == "--mock")
            mock = true;
        else if (arg == "--mock-model")
            mock_model = true;
        else if (arg == "--config" && i + 1 < argc)
            config = argv[++i];
        else if (arg == "--listen" && i + 1 < argc)
            address = argv[++i];
        else {
            std::cerr << "Usage: interview_server [--config path] [--listen host:port] [--mock] "
                         "[--mock-model] [--web-listen host:port] [--web-root path]\n";
            return 2;
        }
    }
    try {
        nlohmann::json cluster = nlohmann::json::object();
        if (!mock) {
            std::ifstream file(config);
            nlohmann::json root;
            file >> root;
            cluster = root.value("cluster", nlohmann::json::object());
        }
        const auto instance = cluster.value("instance_id", std::string{});
        if (!instance.empty() &&
            (instance.size() > 48 ||
             instance.find_first_not_of(
                 "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-") !=
                 std::string::npos))
            throw std::runtime_error("cluster.instance_id 无效");
        auto reports =
            mock ? std::shared_ptr<backend::ReportStore>{} : backend::OpenReportStore(config);
        std::shared_ptr<backend::accounts::Service> accounts;
        if (!mock) {
            std::ifstream file(config);
            nlohmann::json root;
            file >> root;
            accounts =
                backend::accounts::OpenService(root.value("storage", nlohmann::json::object()));
        }
        backend::api::Service service(backend::MakeModel(config, mock || mock_model), reports,
                                      mock ? backend::IdentityResolver{}
                                           : backend::OpenIdentityResolver(config),
                                      accounts, instance);
        std::cout << "Report storage: "
                  << (reports ? "MySQL + Redis cache" : "memory (not persistent)") << std::endl;
        std::shared_ptr<discovery::Registry> registration;
        const auto zk = cluster.value("zookeeper", nlohmann::json::object());
        if (zk.value("enabled", false)) {
            if (instance.empty())
                throw std::runtime_error("启用注册发现必须配置 instance_id");
            registration = std::make_shared<discovery::Registry>(
                zk.value("hosts", std::string("127.0.0.1:2181")),
                zk.value("path", std::string("/aiinterview/backends")),
                discovery::Endpoint{instance, cluster.at("advertised_address")});
            service.SetRouter(std::make_shared<backend::cluster::Router>(
                instance, [registration] { return registration->Snapshot(); }));
        }
        grpc::ServerBuilder builder;
        builder.SetMaxReceiveMessageSize(6 * 1024 * 1024);
        builder.AddListeningPort(address, grpc::InsecureServerCredentials());
        builder.RegisterService(&service);
        auto server = builder.BuildAndStart();
        if (!server)
            throw std::runtime_error("服务监听失败");
        std::cout << "Interview gRPC listening on " << address << (mock ? " [MOCK]" : "")
                  << std::endl;
        backend::web::Server web(service, web_address, web_root);
        std::cout << "Web + gRPC-Web listening on " << web_address << std::endl;
        server->Wait();
    } catch (const std::exception &e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }
}
