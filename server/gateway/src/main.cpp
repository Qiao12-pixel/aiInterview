#include "transport/server.h"
#include <iostream>
int main(int argc, char **argv) {
    try {
        gateway::App app(gateway::LoadConfig(argc, argv));
        gateway::Run(app);
        return 0;
    } catch (const std::exception &e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }
}
