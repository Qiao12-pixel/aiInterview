#pragma once
#include "ports/identity.h"
namespace backend {
IdentityResolver OpenIdentityResolver(const std::string &config_path);
}
