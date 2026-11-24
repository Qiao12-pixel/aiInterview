#pragma once
#include "api/service.h"
namespace backend::web {
std::string Frame(const std::string &data, unsigned char flag = 0);
std::string Unframe(const std::string &data);
std::string Trailers(const grpc::Status &status);
using Emit = std::function<bool(const std::string &)>;
grpc::Status Dispatch(api::Service &, api::Context &, const std::string &method,
                      const std::string &payload, const Emit &);
} // namespace backend::web
