#include "transport/http.h"
#include "transport/sse.h"
namespace gateway {
void ReportRoute(App &app, Stream &stream, const Request &req) {
    const auto body = Body(req, 16 * 1024);
    const auto token = AuthorizeInterview(app, req, body);
    grpc::ClientContext context;
    Configure(context, token, 30);
    Sse output(stream, req, context);
    try {
        const auto result = app.rpc.Report(context, body.at("session_id"));
        output.Send({{"type", "report"}, {"report", result}});
        output.Finish();
    } catch (const std::exception &e) {
        context.TryCancel();
        output.Finish(e.what());
    }
}
void ReportsRoute(App &app, Stream &stream, const Request &req) {
    const auto token = Authenticate(app, req);
    const auto body = Body(req, 4096);
    if (body.contains("offset") &&
        (!body["offset"].is_number_integer() || body["offset"] < 0 || body["offset"] > 10000))
        throw Error(400, "分页参数错误");
    grpc::ClientContext context;
    Configure(context, token, 30);
    JsonResponse(stream, req, 200,
                 {{"reports", app.rpc.Reports(context, body.value("offset", 0u))}});
}
} // namespace gateway
