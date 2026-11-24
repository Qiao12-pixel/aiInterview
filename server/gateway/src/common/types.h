#pragma once
#include <nlohmann/json.hpp>
#include <stdexcept>
namespace gateway {
using Json = nlohmann::json;
class Error : public std::runtime_error {
  public:
    unsigned status;
    Error(unsigned code, const std::string &message) : std::runtime_error(message), status(code) {}
};
} // namespace gateway
