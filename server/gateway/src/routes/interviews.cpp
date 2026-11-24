#include "transport/http.h"
#include "transport/sse.h"
#include "common/codec.h"
namespace gateway {
void CreateInterview(App &app, Stream &stream, const Request &req) {
    const auto token = Authenticate(app, req);
    const auto body = Body(req, 7 * 1024 * 1024);
    ValidateSession(body);
    rpc::CreateSessionRequest message;
    message.set_candidate_name(Trim(body["candidate_name"]));
    message.set_question_total(body["question_total"].get<unsigned>());
    message.set_resume_pdf(Resume(body));
    grpc::ClientContext context;
    Configure(context, token);
    if (Header(req, http::field::accept).find("text/event-stream") == std::string::npos) {
        const auto result = app.rpc.Create(context, message);
        JsonResponse(stream, req, 200,
                     {{"session", SessionJson(result.session())},
                      {"first_question",
                       {{"type", "QUESTION"},
                        {"text", result.first_question().text()},
                        {"session_id", result.session().id()},
                        {"json_payload", result.first_question().json_payload()}}},
                      {"session_token", app.registry.Issue(result.session().id())}});
        return;
    }
    Sse output(stream, req, context);
    try {
        output.Send({{"type", "status"}, {"message", "正在生成面试问题…"}});
        auto reader = app.rpc.Start(context, message);
        rpc::InterviewEvent event;
        while (reader->Read(&event)) {
            if (event.type() == rpc::InterviewEvent::SESSION_READY) {
                auto session = Json::parse(event.json_payload());
                session["type"] = "session";
                session["session_token"] = app.registry.Issue(session.at("id"));
                output.Send(session);
            } else
                output.Send(EventJson(event));
        }
        CheckRpc(reader->Finish());
        output.Finish();
    } catch (const std::exception &e) {
        context.TryCancel();
        output.Finish(e.what());
    }
}
void AnswerInterview(App &app, Stream &stream, const Request &req) {
    const auto body = Body(req, 2 * 1024 * 1024);
    if (!body.contains("text") || !body["text"].is_string() || Trim(body["text"]).empty() ||
        body["text"].get_ref<const std::string &>().size() > 32000)
        throw Error(400, "回答须为 1–32000 字节");
    const auto token = AuthorizeInterview(app, req, body);
    rpc::TextMessage message;
    message.set_session_id(body.at("session_id"));
    message.set_text(Trim(body["text"]));
    message.set_video_jpeg(Frame(body));
    grpc::ClientContext context;
    Configure(context, token);
    Sse output(stream, req, context);
    try {
        output.Send({{"type", "status"}, {"message", "服务端正在分析回答"}});
        auto reader = app.rpc.Answer(context, message);
        rpc::InterviewEvent event;
        while (reader->Read(&event))
            output.Send(EventJson(event));
        CheckRpc(reader->Finish());
        output.Finish();
    } catch (const std::exception &e) {
        context.TryCancel();
        output.Finish(e.what());
    }
}
} // namespace gateway
