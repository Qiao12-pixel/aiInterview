#pragma once
#include "common/types.h"
#include <string>
namespace gateway {
std::string Trim(std::string text);
std::string DecodeBase64(const std::string &value, size_t limit);
std::string EncodeBase64(const std::string &value);
std::string Resume(const Json &body);
std::string Frame(const Json &body);
void ValidateSession(const Json &body);
std::string CookieToken(const std::string &header);
} // namespace gateway
