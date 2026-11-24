#include "common/random.h"
#include "common/codec.h"
#include <stdexcept>
#include <iostream>
void Check(bool value) {
    if (!value)
        throw std::runtime_error("unit assertion failed");
}
int main() {
    using namespace gateway;
    Check(RandomToken().size() == 64);
    Check(RandomToken() != RandomToken());
    Check(CookieToken("x=1; aiinterview_auth=abcdef; z=2") == "abcdef");
    Check(DecodeBase64("YWJj", 100) == "abc");
    Check(EncodeBase64("a") == "YQ==");
    Check(Resume(Json::object()).empty());
    bool failed = false;
    try {
        Resume({{"resume_pdf", "invalid"}});
    } catch (const Error &) {
        failed = true;
    }
    Check(failed);
    std::cout << "random tokens, cookies, Base64 and upload validation passed\n";
}
