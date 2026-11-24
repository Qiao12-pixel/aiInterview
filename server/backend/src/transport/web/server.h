#pragma once
#include "api/service.h"
#include <memory>
namespace backend::web {
class Server {
    struct Impl;
    std::unique_ptr<Impl> impl_;

  public:
    Server(api::Service &, const std::string &address, const std::string &root,
           unsigned workers = 32);
    ~Server();
};
} // namespace backend::web
