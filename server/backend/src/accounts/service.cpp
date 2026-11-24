#include "accounts/service.h"
#include "infrastructure/database/bootstrap.h"
#include "accounts/crypto.h"

#include <algorithm>
#include <regex>
namespace backend::accounts {
namespace {
std::string Trim(const std::string &value) {
    auto a = value.find_first_not_of(" \r\n\t"), b = value.find_last_not_of(" \r\n\t");
    return a == std::string::npos ? "" : value.substr(a, b - a + 1);
}

size_t Utf16Length(const std::string &text) {
    size_t result = 0;
    for (unsigned char ch : text)
        if ((ch & 0xc0) != 0x80)
            result += ch >= 0xf0 ? 2 : 1;
    return result;
}
std::string Email(const Json &body) {
    if (!body.contains("email") || !body["email"].is_string())
        throw Error(400, "请输入有效邮箱");
    auto email = Trim(body["email"]);
    std::transform(email.begin(), email.end(), email.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    if (email.size() > 254 || !std::regex_match(email, std::regex("[^\\s@]+@[^\\s@]+\\.[^\\s@]+")))
        throw Error(400, "请输入有效邮箱");
    return email;
}
} // namespace
Service::Service(std::unique_ptr<UserRepository> users, std::unique_ptr<SessionRepository> sessions,
                 unsigned ttl)
    : users_(std::move(users)), sessions_(std::move(sessions)), ttl_(ttl) {
    if (ttl < 60 || ttl > 30 * 86400)
        throw Error(503, "登录有效期须为 60 秒到 30 天");
}
Json Service::CreateSession(Json user) {
    user.erase("password_hash");
    const auto token = RandomHex(32);
    sessions_->Save(token, user, ttl_);
    return {{"user", user}, {"token", token}};
}
Json Service::Register(const Json &body) {
    const auto email = Email(body);
    if (!body.contains("password") || !body["password"].is_string() ||
        !body.contains("display_name") || !body["display_name"].is_string())
        throw Error(400, "请填写密码和昵称");
    const std::string password = body["password"], name = Trim(body["display_name"]);
    if (Utf16Length(password) < 10 || Utf16Length(password) > 128 || name.empty() ||
        Utf16Length(name) > 80)
        throw Error(400, "请输入昵称和至少 10 位密码");
    Json user = {{"id", RandomHex(16)}, {"email", email}, {"display_name", name}};
    users_->Insert(user, HashPassword(password));
    return CreateSession(user);
}
Json Service::Login(const Json &body) {
    const auto email = Email(body);
    if (!body.contains("password") || !body["password"].is_string() ||
        Utf16Length(body["password"].get_ref<const std::string &>()) > 128)
        throw Error(401, "邮箱或密码错误");
    const auto user = users_->Find(email);
    const std::string fallback = "scrypt$00000000000000000000000000000000$" + std::string(128, '0');
    const bool valid = VerifyPassword(
        body["password"], user ? user->at("password_hash").get<std::string>() : fallback);
    if (!user || !valid)
        throw Error(401, "邮箱或密码错误");
    return CreateSession(*user);
}
Json Service::Authenticate(const std::string &token) {
    auto user = sessions_->Find(token);
    if (!user)
        throw Error(401, "请先登录或重新登录");
    return *user;
}
void Service::Logout(const std::string &token) { sessions_->Delete(token); }
void Service::Limit(const std::string &source) {
    if (!sessions_->AllowAttempt(source))
        throw Error(429, "尝试过多，请 15 分钟后重试");
}
std::unique_ptr<Service> OpenService(const Json &config) {
    if (!config.value("enabled", false))
        return {};
    backend::database::Initialize(config.at("mysql"));
    return std::make_unique<Service>(
        MysqlUsers(config.at("mysql")), RedisSessions(config.value("redis", Json::object())),
        config.value("accounts", Json::object()).value("session_ttl_seconds", 604800));
}
} // namespace backend::accounts
