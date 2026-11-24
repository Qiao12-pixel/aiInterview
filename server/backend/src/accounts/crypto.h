#pragma once
#include <string>
namespace backend::accounts {
std::string RandomHex(size_t bytes);
std::string Digest(const std::string &value);
std::string HashPassword(const std::string &password);
bool VerifyPassword(const std::string &password, const std::string &stored);
} // namespace backend::accounts
