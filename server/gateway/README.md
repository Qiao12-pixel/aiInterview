# WebSocket → gRPC 网关

网关属于服务端，负责 HTTP 静态资源、WebSocket 协议转换和请求生命周期，不负责出题、评分或状态机。

从项目根目录启动：

```sh
npm ci --prefix server/gateway
npm start --prefix server/gateway
```

默认监听 `127.0.0.1:8080`，上游 `127.0.0.1:50051`。可配置 `PORT`、`HOST`、`GRPC_TARGET`、`ALLOWED_ORIGIN`、`WEB_ROOT`、`PROTO_PATH`。路径默认相对于模块位置定位，不依赖启动目录。

源文件职责：

- `server.js`：启动和关闭服务、组装依赖
- `config.js`：环境配置与路径
- `grpc-client.js`：加载协议、创建 RPC 客户端
- `http.js`：静态资源和健康检查
- `answer-route.js`、`report-route.js`：独立 POST/SSE 端点
- `sse-writer.js`、`request-body.js`、`session-auth.js`：SSE 生命周期、请求边界和会话校验
- `http-response.js`：统一 JSON 错误响应
- `websocket.js`：连接状态、参数校验与 RPC 消息桥接，接收注入的客户端

先启动 `--mock --listen 127.0.0.1:50061` 的 C++ 后端，再用 `GRPC_TARGET=127.0.0.1:50061 PORT=8081` 启动网关，然后运行 `npm test --prefix server/gateway`。
