// //
// // Created by Joe on 26-4-1.
// //
// #include <iostream>
// #include <boost/beast.hpp>
// #include <boost/asio.hpp>
// #include <boost/beast/websocket.hpp>
// #include <boost/beast/core.hpp>
// #include <spdlog/spdlog.h>
// //替换命名空间
// namespace beast = boost::beast;
// namespace http = beast::http;
// namespace websocket = beast::websocket;
// namespace net = boost::asio;
// using tcp = boost::asio::ip::tcp;
// int main() {
//     spdlog::info("websocket服务端启动");
//     try {
//         auto const address = net::ip::make_address("127.0.0.1");
//         auto const port = static_cast<unsigned int>(8080);
//
//         net::io_context ioc{1};
//         tcp::acceptor acceptor{ioc, {address, port}};
//
//         while (true) {
//             tcp::socket socket{ioc};
//             acceptor.accept(socket);
//
//             websocket::stream<tcp::socket> ws{std::move(socket)};
//
//             ws.accept();
//             spdlog::info("websocket与客户端已建立链接");
//             for (;;) {
//                 beast::flat_buffer buffer;
//
//                 ws.read(buffer);
//                 ws.text(ws.got_text());
//                 spdlog::info("wesocket服务端收到信息: {}", beast::buffers_to_string(buffer.data()));
//                 ws.write(buffer.data());
//             }
//         }
//     } catch (beast::system_error const& se) {
//         if (se.code() == websocket::error::closed) {
//             spdlog::info("客户端正常断开了链接(close Frame)");
//         } else {
//             spdlog::error("客户端异常断开{}", se.code().message());
//         }
//     }
// }