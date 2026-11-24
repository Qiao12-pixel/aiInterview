#include "accounts/service.h"
#include "accounts/crypto.h"
#include <unordered_map>
#include <iostream>
namespace {
using namespace backend::accounts;
struct Users : UserRepository {
    std::unordered_map<std::string, Json> values;
    void Insert(const Json &user, const std::string &hash) override {
        auto email = user.at("email").get<std::string>();
        if (values.count(email))
            throw Error(409, "duplicate");
        values[email] = user;
        values[email]["password_hash"] = hash;
    }
    std::optional<Json> Find(const std::string &email) override {
        auto found = values.find(email);
        if (found == values.end())
            return {};
        return found->second;
    }
};
struct Sessions : SessionRepository {
    std::unordered_map<std::string, Json> values;
    void Save(const std::string &token, const Json &user, unsigned) override {
        values[token] = user;
    }
    std::optional<Json> Find(const std::string &token) override {
        auto found = values.find(token);
        if (found == values.end())
            return {};
        return found->second;
    }
    void Delete(const std::string &token) override { values.erase(token); }
    bool AllowAttempt(const std::string &) override { return true; }
};
void Check(bool value) {
    if (!value)
        throw std::runtime_error("账号测试失败");
}
} // namespace
int main() {
    auto users = std::make_unique<Users>();
    auto *stored = users.get();
    backend::accounts::Service service(std::move(users), std::make_unique<Sessions>(), 600);
    const Json body = {
        {"email", "User@example.test"}, {"password", "Password123456!"}, {"display_name", "测试"}};
    auto result = service.Register(body);
    const auto token = result.at("token").get<std::string>();
    Check(token.size() == 64);
    Check(!result.at("user").contains("password_hash"));
    Check(service.Authenticate(token).at("email") == "user@example.test");
    Check(stored->values.at("user@example.test")
              .at("password_hash")
              .get<std::string>()
              .rfind("scrypt$", 0) == 0);
    service.Logout(token);
    try {
        service.Authenticate(token);
        return 1;
    } catch (const Error &e) {
        Check(e.status == 401);
    }
    auto login = service.Login(body);
    Check(login.at("user").at("id") == result.at("user").at("id"));
    auto wrong = body;
    wrong["password"] = "incorrect";
    try {
        service.Login(wrong);
        return 2;
    } catch (const Error &e) {
        Check(e.status == 401);
    }
    try {
        service.Register(body);
        return 3;
    } catch (const Error &e) {
        Check(e.status == 409);
    }
    std::cout << "Backend account register/login/logout and password isolation passed\n";
}
