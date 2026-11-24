#include "api/service.h"
namespace backend::api {
namespace rpc = aiinterview::v1;
grpc::Status Service::Account(grpc::ServerContext *context, const rpc::AccountRequest *req,
                              rpc::AccountResponse *res) {
    try {
        if (!accounts_) {
            if (req->operation() == "me") {
                res->set_result_json("{\"enabled\":false}");
                return grpc::Status::OK;
            }
            return {grpc::StatusCode::UNAVAILABLE, "账号服务未配置"};
        }
        auto body = Json::parse(req->body_json().empty() ? "{}" : req->body_json(), nullptr, false);
        if (req->body_json().size() > 16 * 1024 || !body.is_object())
            return {grpc::StatusCode::INVALID_ARGUMENT, "请求必须是 JSON 对象且不超过 16 KiB"};
        Json result;
        const auto operation = req->operation();
        if (operation == "register" || operation == "login") {
            // 来源由网关适配请求的 socket 地址，后端 peer 合并限流，不能通过不同 operation 绕过。
            auto peer = context->peer();
            const auto last = peer.rfind(':');
            if (last != std::string::npos)
                peer.resize(last);
            accounts_->Limit(peer + ":" + req->source().substr(0, 256));
            result = operation == "register" ? accounts_->Register(body) : accounts_->Login(body);
            result["ttl_seconds"] = accounts_->ttl();
            result["enabled"] = true;
        } else if (operation == "me" || operation == "logout") {
            const auto it = context->client_metadata().find("authorization");
            std::string token;
            if (it != context->client_metadata().end())
                token.assign(it->second.data(), it->second.size());
            if (token.rfind("Bearer ", 0) != 0)
                throw accounts::Error(401, "请先登录");
            token = token.substr(7);
            const auto user = accounts_->Authenticate(token);
            if (operation == "logout") {
                accounts_->Logout(token);
                result = {{"ok", true}, {"enabled", true}};
            } else
                result = {{"user", user}, {"enabled", true}};
        } else
            return {grpc::StatusCode::NOT_FOUND, "账号接口不存在"};
        res->set_result_json(result.dump());
        return grpc::Status::OK;
    } catch (const accounts::Error &e) {
        auto code = grpc::StatusCode::UNAVAILABLE;
        if (e.status == 400)
            code = grpc::StatusCode::INVALID_ARGUMENT;
        else if (e.status == 401)
            code = grpc::StatusCode::UNAUTHENTICATED;
        else if (e.status == 409)
            code = grpc::StatusCode::ALREADY_EXISTS;
        else if (e.status == 429)
            code = grpc::StatusCode::RESOURCE_EXHAUSTED;
        return {code, e.what()};
    } catch (const Json::exception &) {
        return {grpc::StatusCode::INVALID_ARGUMENT, "账号字段格式无效"};
    } catch (const std::exception &) {
        return {grpc::StatusCode::UNAVAILABLE, "账号服务暂不可用"};
    }
}
} // namespace backend::api
