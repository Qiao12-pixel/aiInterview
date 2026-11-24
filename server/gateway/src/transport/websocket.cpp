#include "transport/websocket.h"
#include "common/codec.h"
#include <boost/beast/websocket.hpp>
namespace gateway {
void HandleWebSocket(App &app, Stream stream, Request req) {
    namespace ws = beast::websocket;
    if (Path(req) != "/ws" || !AllowedOrigin(app, req)) {
        JsonResponse(stream, req, 403, {{"message", "Origin 不匹配"}});
        return;
    }
    std::string token;
    try {
        token = Authenticate(app, req);
    } catch (const Error &e) {
        JsonResponse(stream, req, e.status, {{"message", e.what()}});
        return;
    }
    ws::stream<Stream> socket(std::move(stream));
    socket.read_message_max(64 * 1024);
    socket.set_option(ws::stream_base::timeout::suggested(beast::role_type::server));
    boost::system::error_code ec;
    socket.accept(req, ec);
    if (ec)
        return;
    const auto send = [&](const Json &value) {
        socket.text(true);
        socket.write(net::buffer(value.dump()));
    };
    try {
        send({{"type", "connected"}, {"message", "已连接面试服务网关"}});
        std::string id;
        while (true) {
            beast::flat_buffer buffer;
            socket.read(buffer, ec);
            if (ec)
                break;
            Json requestId = nullptr;
            try {
                if (!socket.got_text())
                    throw Error(400, "只接受 JSON 文本消息");
                const auto body =
                    Json::parse(beast::buffers_to_string(buffer.data()), nullptr, false);
                if (!body.is_object() || !body.contains("type") || !body["type"].is_string())
                    throw Error(400, "消息必须是包含 type 的 JSON 对象");
                requestId = body.value("request_id", Json{});
                app.rpc.Account("me", Json::object(), token);
                const auto kind = body["type"].get<std::string>();
                grpc::ClientContext ctx;
                Configure(ctx, token);
                if (kind == "create_session") {
                    ValidateSession(body);
                    rpc::CreateSessionRequest create;
                    create.set_candidate_name(Trim(body["candidate_name"]));
                    create.set_question_total(body["question_total"].get<unsigned>());
                    const auto result = app.rpc.Create(ctx, create);
                    id = result.session().id();
                    auto session = SessionJson(result.session());
                    session["type"] = "session";
                    session["session_token"] = app.registry.Issue(id);
                    session["request_id"] = requestId;
                    send(session);
                    auto event = EventJson(result.first_question());
                    event["request_id"] = requestId;
                    send(event);
                } else if (kind == "answer") {
                    if (id.empty() || !body.contains("text") || !body["text"].is_string() ||
                        Trim(body["text"]).empty())
                        throw Error(400, "请先创建会话并输入回答");
                    rpc::TextMessage text;
                    text.set_session_id(id);
                    text.set_text(Trim(body["text"]));
                    auto reader = app.rpc.Answer(ctx, text);
                    rpc::InterviewEvent event;
                    while (reader->Read(&event)) {
                        auto value = EventJson(event);
                        value["request_id"] = requestId;
                        send(value);
                    }
                    CheckRpc(reader->Finish());
                } else if (kind == "get_report") {
                    if (id.empty())
                        throw Error(400, "请先创建会话");
                    send({{"type", "report"},
                          {"request_id", requestId},
                          {"report", app.rpc.Report(ctx, id)}});
                } else
                    throw Error(400, "不支持的消息类型");
                send({{"type", "done"}, {"request_id", requestId}});
            } catch (const std::exception &e) {
                send({{"type", "error"}, {"request_id", requestId}, {"message", e.what()}});
            }
        }
    } catch (const boost::system::system_error &) {
    }
}
} // namespace gateway
