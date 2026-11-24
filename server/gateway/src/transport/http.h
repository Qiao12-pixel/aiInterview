#pragma once
#include "app.h"
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
namespace gateway {
namespace net = boost::asio;
namespace beast = boost::beast;
namespace http = beast::http;
using Request = http::request<http::string_body>;
using Stream = beast::tcp_stream;
std::string Header(const Request &req, http::field name);
std::string Path(const Request &req);
Json Body(const Request &req, size_t limit);
void JsonResponse(Stream &stream, const Request &req, unsigned status, const Json &body,
                  const std::string &cookie = {});
std::string Authenticate(App &app, const Request &req);
std::string AuthorizeInterview(App &app, const Request &req, const Json &body);
bool AllowedOrigin(const App &app, const Request &req);
void HandleHttp(App &app, Stream &stream, Request &req);
void StaticFile(App &app, Stream &stream, const Request &req);
void AccountRoute(App &app, Stream &stream, const Request &req);
void CreateInterview(App &app, Stream &stream, const Request &req);
void AnswerInterview(App &app, Stream &stream, const Request &req);
void ReportRoute(App &app, Stream &stream, const Request &req);
void ReportsRoute(App &app, Stream &stream, const Request &req);
} // namespace gateway
