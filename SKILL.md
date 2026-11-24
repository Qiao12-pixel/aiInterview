# 项目开发工作流

1. 阅读 AGENTS.md、docs/ARCHITECTURE.md，确定变更所属模块。
2. 界面分别放 client/ 和 web/；业务全部放 server/backend/，不新增独立网关。
3. Qt 使用原生 gRPC，浏览器使用后端内置的 gRPC-Web。两种传输调用同一个 api::Service，与 application/Engine 共用业务逻辑，不经本机 gRPC 再转发。
4. 协议调整同步 proto/、C++、web/src/rpc/。目前保留文本与浏览器语音转写，不提供视频和 WebSocket。
5. 外部依赖通过 ports/ 隔离，具体实现位于 infrastructure/；头文件与实现同目录。
6. 使用 RAII，连接池与网络工作队列有界；耗时业务不阻塞 HTTP I/O 线程。不重试已提交的回答。
7. 修改后构建与运行相关测试，更新 startup.md 和架构文档，检查敏感信息。

模型上游仍使用 SSE；浏览器流式输出使用 gRPC-Web protobuf 数据帧与最终状态 trailer，缺少 trailer 不视为成功。
