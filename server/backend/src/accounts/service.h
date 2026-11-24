#pragma once
#include "accounts/repositories.h"
namespace backend::accounts {
class Service {
    std::unique_ptr<UserRepository> users_;
    std::unique_ptr<SessionRepository> sessions_;
    unsigned ttl_;
    Json CreateSession(Json user);

  public:
    Service(std::unique_ptr<UserRepository> users, std::unique_ptr<SessionRepository> sessions,
            unsigned ttl);
    Json Register(const Json &body);
    Json Login(const Json &body);
    Json Authenticate(const std::string &token);
    void Logout(const std::string &token);
    void Limit(const std::string &source);
    unsigned ttl() const { return ttl_; }
};
std::unique_ptr<Service> OpenService(const Json &storage);
} // namespace backend::accounts
