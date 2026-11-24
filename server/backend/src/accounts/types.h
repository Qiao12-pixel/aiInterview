#pragma once
#include <nlohmann/json.hpp>
#include <stdexcept>
namespace backend::accounts {
using Json = nlohmann::json;
class Error : public std::runtime_error {
  public:
    unsigned status;
    Error(unsigned code, const std::string &text) : std::runtime_error(text), status(code) {}
};
} // namespace backend::accounts
