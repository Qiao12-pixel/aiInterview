#pragma once
#include <functional>
#include <string>
namespace backend {
using IdentityResolver = std::function<std::string(const std::string &)>;
}
