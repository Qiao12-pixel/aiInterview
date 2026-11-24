#include "common/random.h"
#include <openssl/rand.h>
#include <stdexcept>
namespace gateway {
std::string RandomToken() {
    unsigned char bytes[32];
    if (RAND_bytes(bytes, sizeof(bytes)) != 1)
        throw std::runtime_error("随机数生成失败");
    constexpr char hex[] = "0123456789abcdef";
    std::string out;
    out.reserve(64);
    for (auto byte : bytes) {
        out += hex[byte >> 4];
        out += hex[byte & 15];
    }
    return out;
}
} // namespace gateway
