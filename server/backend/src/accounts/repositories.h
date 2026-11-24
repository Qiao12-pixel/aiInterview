#pragma once
#include "accounts/types.h"
#include <optional>
#include <memory>
namespace backend::accounts {
class UserRepository {
  public:
    virtual ~UserRepository() = default;
    virtual void Insert(const Json &user, const std::string &password_hash) = 0;
    virtual std::optional<Json> Find(const std::string &email) = 0;
};
class SessionRepository {
  public:
    virtual ~SessionRepository() = default;
    virtual void Save(const std::string &token, const Json &user, unsigned ttl) = 0;
    virtual std::optional<Json> Find(const std::string &token) = 0;
    virtual void Delete(const std::string &token) = 0;
    virtual bool AllowAttempt(const std::string &key) = 0;
};
std::unique_ptr<UserRepository> MysqlUsers(const Json &config);
std::unique_ptr<SessionRepository> RedisSessions(const Json &config);
} // namespace backend::accounts
