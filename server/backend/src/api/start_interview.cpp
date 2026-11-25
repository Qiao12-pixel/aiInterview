#include "api/service.h"
#include "api/routing.h"
#include "infrastructure/pdf_parser.h"
#include <stdexcept>
namespace backend::api {
namespace rpc = aiinterview::v1;
grpc::Status Service::StartInterview(Context *context, const rpc::CreateSessionRequest *req,
                                     Writer<rpc::InterviewEvent> *writer) {
    if (auto routed = RouteStart(router_.get(), context, req, writer))
        return *routed;
    std::string owner;
    try {
        owner = User(context);
    } catch (const std::invalid_argument &e) {
        return {grpc::StatusCode::UNAUTHENTICATED, e.what()};
    } catch (const std::exception &e) {
        return {grpc::StatusCode::UNAVAILABLE, e.what()};
    }
    const auto write = [&](rpc::InterviewEvent::Type type, const std::string &text,
                           const Json &payload, const std::string &id = {}) {
        rpc::InterviewEvent event;
        event.set_type(type);
        event.set_text(text);
        event.set_session_id(id);
        event.set_json_payload(payload.dump());
        if (context->IsCancelled() || !writer->Write(event))
            throw std::runtime_error("客户端已断开，生成中止");
    };
    try {
        const auto resume = ParseResumePdf(req->resume_pdf());
        const auto session = engine_.Create(
            req->candidate_name(), req->question_total(), resume, owner, [&](const Event &event) {
                write(rpc::InterviewEvent::DELTA, event.text, event.payload);
            });
        write(rpc::InterviewEvent::SESSION_READY, "",
              {{"id", session->id},
               {"candidate_name", session->candidate},
               {"status", "questioning"},
               {"question_index", 1},
               {"question_total", session->questions.size()}},
              session->id);
        write(rpc::InterviewEvent::QUESTION, session->current,
              {{"question_index", 1}, {"question_total", session->questions.size()}}, session->id);
        return grpc::Status::OK;
    } catch (const std::invalid_argument &e) {
        return {grpc::StatusCode::INVALID_ARGUMENT, e.what()};
    } catch (const std::exception &e) {
        return {grpc::StatusCode::UNAVAILABLE, e.what()};
    }
}
} // namespace backend::api
