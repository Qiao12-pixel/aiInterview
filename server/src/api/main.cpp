#include "interview/api/service.h"
#include "interview/infrastructure/llm.h"
#include <grpcpp/grpcpp.h>
#include <iostream>
int main(int argc, char **argv) {
    std::string config = "server/config/default_config.json", address = "127.0.0.1:50051";
    bool mock = false;
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--mock")
            mock = true;
        else if (arg == "--config" && i + 1 < argc)
            config = argv[++i];
        else if (arg == "--listen" && i + 1 < argc)
            address = argv[++i];
        else {
            std::cerr << "Usage: interview_server [--config path] [--listen host:port] [--mock]\n";
            return 2;
        }
    }
    try {
        backend::api::Service service(backend::MakeModel(config, mock));
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
