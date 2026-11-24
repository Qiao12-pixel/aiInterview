#include "interview/api/service.h"
#include "interview/pdf_parser.h"
#include "interview/common/base64.h"
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
Service::Service(Model model) : engine_(std::move(model)) {}
grpc::Status Service::CreateSession(grpc::ServerContext *, const rpc::CreateSessionRequest *req,
                                    rpc::CreateSessionResponse *res) {
    try {
        auto s = engine_.Create(req->candidate_name(), req->question_total(),
                                backend::ParseResumePdf(req->resume_pdf()));
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
grpc::Status Service::SendText(grpc::ServerContext *, const rpc::TextMessage *req,
                               grpc::ServerWriter<rpc::InterviewEvent> *writer) {
    auto s = engine_.Find(req->session_id());
    if (!s)
        return {grpc::StatusCode::NOT_FOUND, "会话不存在或服务已重启"};
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
grpc::Status Service::GetReport(grpc::ServerContext *, const rpc::ReportRequest *req,
                                rpc::ReportResponse *res) {
    auto s = engine_.Find(req->session_id());
    if (!s)
        return {grpc::StatusCode::NOT_FOUND, "会话不存在"};
    std::unique_lock<std::mutex> lock(s->mutex, std::try_to_lock);
    if (!lock.owns_lock())
        return {grpc::StatusCode::RESOURCE_EXHAUSTED, "会话正在处理请求"};
    if (!s->completed)
        return {grpc::StatusCode::FAILED_PRECONDITION, "面试尚未结束"};
    res->set_session_id(s->id);
    res->set_report_json(s->report.dump());
    return grpc::Status::OK;
}
} // namespace backend::api
