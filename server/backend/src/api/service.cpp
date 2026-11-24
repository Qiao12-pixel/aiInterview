#include "api/service.h"
#include "infrastructure/pdf_parser.h"
#include "common/base64.h"
#include <stdexcept>
namespace backend::api {
namespace rpc = aiinterview::v1;
namespace {
grpc::Status Failure(const std::exception &e) {
    return {dynamic_cast<const std::invalid_argument *>(&e) ? grpc::StatusCode::INVALID_ARGUMENT
                                                            : grpc::StatusCode::UNAVAILABLE,
            e.what()};
}
} // namespace
Service::Service(Model model, std::shared_ptr<ReportStore> reports, IdentityResolver identity,
                 std::shared_ptr<accounts::Service> accounts)
    : engine_(std::move(model), std::move(reports)), identity_(std::move(identity)),
      accounts_(std::move(accounts)) {}
grpc::Status Service::CreateSession(grpc::ServerContext *context,
                                    const rpc::CreateSessionRequest *req,
                                    rpc::CreateSessionResponse *res) {
    std::string owner;
    try {
        owner = User(context);
    } catch (const std::invalid_argument &e) {
        return {grpc::StatusCode::UNAUTHENTICATED, e.what()};
    } catch (const std::exception &e) {
        return {grpc::StatusCode::UNAVAILABLE, e.what()};
    }
    try {
        auto s = engine_.Create(req->candidate_name(), req->question_total(),
                                backend::ParseResumePdf(req->resume_pdf()), owner);
        auto *session = res->mutable_session();
        session->set_id(s->id);
        session->set_candidate_name(s->candidate);
        session->set_status("questioning");
        session->set_question_index(1);
        session->set_question_total(s->questions.size());
        auto *first = res->mutable_first_question();
        first->set_type(rpc::InterviewEvent::QUESTION);
        first->set_session_id(s->id);
        first->set_text(s->current);
        return grpc::Status::OK;
    } catch (const std::exception &e) {
        return Failure(e);
    }
}
grpc::Status Service::SendText(grpc::ServerContext *context, const rpc::TextMessage *req,
                               grpc::ServerWriter<rpc::InterviewEvent> *writer) {
    std::string user;
    try {
        user = User(context);
    } catch (const std::invalid_argument &e) {
        return {grpc::StatusCode::UNAUTHENTICATED, e.what()};
    } catch (const std::exception &e) {
        return {grpc::StatusCode::UNAVAILABLE, e.what()};
    }
    auto s = engine_.Find(req->session_id());
    if (!s)
        return {grpc::StatusCode::NOT_FOUND, "会话不存在或服务已重启"};
    if (identity_ && s->owner != user)
        return {grpc::StatusCode::PERMISSION_DENIED, "无权访问此会话"};
    std::unique_lock<std::mutex> lock(s->mutex, std::try_to_lock);
    if (!lock.owns_lock())
        return {grpc::StatusCode::RESOURCE_EXHAUSTED, "该会话正在处理其他请求"};
    try {
        const auto write = [&](const backend::Event &event) {
            rpc::InterviewEvent out;
            rpc::InterviewEvent::Type type = rpc::InterviewEvent::TYPE_UNSPECIFIED;
            if (!rpc::InterviewEvent::Type_Parse(event.type, &type))
                throw std::runtime_error("未知事件类型");
            out.set_type(type);
            out.set_session_id(s->id);
            out.set_text(event.text);
            out.set_json_payload(event.payload.dump());
            if (!writer->Write(out))
                throw std::runtime_error("客户端已断开，生成中止");
        };
        if (req->video_jpeg().size() > 512 * 1024 ||
            (!req->video_jpeg().empty() &&
             (req->video_jpeg().size() < 2 ||
              static_cast<unsigned char>(req->video_jpeg()[0]) != 0xff ||
              static_cast<unsigned char>(req->video_jpeg()[1]) != 0xd8)))
            throw std::invalid_argument("视频画面必须为不超过 512 KiB 的 JPEG");
        const auto frame = common::Base64(req->video_jpeg());
        for (const auto &event : engine_.Answer(*s, req->text(), write, frame))
            write(event);
        return grpc::Status::OK;
    } catch (const std::exception &e) {
        return Failure(e);
    }
}
grpc::Status Service::GetReport(grpc::ServerContext *context, const rpc::ReportRequest *req,
                                rpc::ReportResponse *res) {
    try {
        const auto user = User(context);
        const auto report = engine_.Report(req->session_id());
        if (!report)
            return {grpc::StatusCode::NOT_FOUND, "报告不存在或面试尚未结束"};
        if (identity_ && report->value("owner_user_id", std::string{}) != user)
            return {grpc::StatusCode::NOT_FOUND, "报告不存在"};
        res->set_session_id(req->session_id());
        res->set_report_json(report->dump());
        return grpc::Status::OK;
    } catch (const std::exception &e) {
        return Failure(e);
    }
}
grpc::Status Service::ListReports(grpc::ServerContext *context, const rpc::ListReportsRequest *req,
                                  rpc::ListReportsResponse *res) {
    try {
        const auto user = User(context);
        if (user.empty())
            return {grpc::StatusCode::UNAUTHENTICATED, "请先登录"};
        res->set_reports_json(engine_.ListReports(user, req->offset()).dump());
        return grpc::Status::OK;
    } catch (const std::invalid_argument &e) {
        return {grpc::StatusCode::UNAUTHENTICATED, e.what()};
    } catch (const std::exception &e) {
        return {grpc::StatusCode::UNAVAILABLE, e.what()};
    }
}
std::string Service::User(grpc::ServerContext *context) {
    if (!identity_)
        return {};
    const auto it = context->client_metadata().find("authorization");
    if (it == context->client_metadata().end())
        throw std::invalid_argument("请先登录");
    std::string token(it->second.data(), it->second.size());
    if (token.rfind("Bearer ", 0) != 0)
        throw std::invalid_argument("登录凭据无效");
    const auto user = identity_(token.substr(7));
    if (user.empty())
        throw std::invalid_argument("登录已过期，请重新登录");
    return user;
}
} // namespace backend::api
