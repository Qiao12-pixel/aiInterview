#include "accounts/crypto.h"
#include <openssl/rand.h>
#include <openssl/sha.h>
#include <openssl/evp.h>
#include <openssl/crypto.h>
#include <array>
#include <stdexcept>
namespace backend::accounts {
namespace {
std::string Hex(const unsigned char *value, size_t size) {
    constexpr char alphabet[] = "0123456789abcdef";
    std::string out;
    out.reserve(size * 2);
    for (size_t i = 0; i < size; ++i) {
        out += alphabet[value[i] >> 4];
        out += alphabet[value[i] & 15];
    }
    return out;
}
std::array<unsigned char, 64> Derive(const std::string &password, const std::string &salt) {
    std::array<unsigned char, 64> out{};
    if (!EVP_PBE_scrypt(password.data(), password.size(),
                        reinterpret_cast<const unsigned char *>(salt.data()), salt.size(), 32768, 8,
                        1, 64 * 1024 * 1024, out.data(), out.size()))
        throw std::runtime_error("密码哈希失败");
    return out;
}
} // namespace
std::string RandomHex(size_t size) {
    std::string bytes(size, '\0');
    if (RAND_bytes(reinterpret_cast<unsigned char *>(bytes.data()), size) != 1)
        throw std::runtime_error("随机数生成失败");
    return Hex(reinterpret_cast<const unsigned char *>(bytes.data()), size);
}
std::string Digest(const std::string &value) {
    unsigned char out[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char *>(value.data()), value.size(), out);
    return Hex(out, sizeof(out));
}
std::string HashPassword(const std::string &password) {
    const auto salt = RandomHex(16);
    const auto key = Derive(password, salt);
    return "scrypt$" + salt + "$" + Hex(key.data(), key.size());
}
bool VerifyPassword(const std::string &password, const std::string &stored) {
    if (stored.rfind("scrypt$", 0) != 0)
        return false;
    const auto split = stored.find('$', 7);
    if (split == std::string::npos)
        return false;
    const auto salt = stored.substr(7, split - 7), expected = stored.substr(split + 1);
    if (salt.size() != 32 || expected.size() != 128 ||
        expected.find_first_not_of("0123456789abcdef") != std::string::npos)
        return false;
    const auto key = Derive(password, salt);
    const auto actual = Hex(key.data(), key.size());
    return CRYPTO_memcmp(actual.data(), expected.data(), actual.size()) == 0;
}
} // namespace backend::accounts
