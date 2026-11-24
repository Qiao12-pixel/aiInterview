#include "interview/common/base64.h"
namespace backend::common {
std::string Base64(const std::string &bytes) {
    static constexpr char alphabet[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string out;
    unsigned value = 0;
    int bits = -6;
    for (unsigned char ch : bytes) {
        value = (value << 8) | ch;
        bits += 8;
        while (bits >= 0) {
            out += alphabet[(value >> bits) & 63];
            bits -= 6;
        }
    }
    if (bits > -6)
        out += alphabet[((value << 8) >> (bits + 8)) & 63];
    while (out.size() % 4)
        out += '=';
    return out;
}
} // namespace backend::common
