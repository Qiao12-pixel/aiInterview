#include "api/service.h"
#include "infrastructure/llm.h"
#include <grpcpp/grpcpp.h>
#include <iostream>
#include "infrastructure/storage/report_store.h"
#include "infrastructure/storage/identity.h"
#include <fstream>
int main(int argc, char **argv) {
    std::string config = "server/config/default_config.json", address = "127.0.0.1:50051";
    bool mock = false, mock_model = false;
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--mock")
            mock = true;
        else if (arg == "--mock-model")
            mock_model = true;
        else if (arg == "--config" && i + 1 < argc)
            config = argv[++i];
        else if (arg == "--listen" && i + 1 < argc)
            address = argv[++i];
        else {
            std::cerr << "Usage: interview_server [--config path] [--listen host:port] [--mock] "
                         "[--mock-model]\n";
            return 2;
        }
    }
    try {
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
        backend::api::Service service(
            backend::MakeModel(config, mock || mock_model), reports,
            mock ? backend::IdentityResolver{} : backend::OpenIdentityResolver(config), accounts);
        std::cout << "Report storage: "
                  << (reports ? "MySQL + Redis cache" : "memory (not persistent)") << std::endl;
        grpc::ServerBuilder builder;
        builder.SetMaxReceiveMessageSize(6 * 1024 * 1024);
        builder.AddListeningPort(address, grpc::InsecureServerCredentials());
        builder.RegisterService(&service);
        auto server = builder.BuildAndStart();
        if (!server)
            throw std::runtime_error("服务监听失败");
        std::cout << "Interview gRPC listening on " << address << (mock ? " [MOCK]" : "")
                  << std::endl;
        server->Wait();
    } catch (const std::exception &e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }
}
