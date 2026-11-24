# 架构与依赖边界

单个 interview_server 进程同时提供原生 gRPC（默认 50051）和网页/gRPC-Web（默认 8888）。不再存在 server/gateway、独立协议代理或 WebSocket。浏览器受 Web API 限制，使用 gRPC-Web，不是原生 HTTP/2 gRPC。

```text
Qt ──原生 gRPC──────────┐
                       api::Service → application/Engine → ports → infrastructure
Web ──gRPC-Web──────────┘                                  ├─ 火山模型（上游 SSE）
     transport/web                                      ├─ MySQL / Redis 连接池
                                                        └─ MySQL outbox → RabbitMQ → 报告任务
```

transport/web 仅处理 HTTP、gRPC-Web framing、Cookie 和静态资源，直接调用同进程 api::Service，不建立本机 gRPC client 连接。api/native.cpp 将原生 gRPC 上下文适配成相同服务接口。账号模块负责验证、scrypt、登录态和限流，应用层负责出题、追问、评分及报告。

## 流式输出

StartInterview / SendText 是服务端流。浏览器上传 protobuf 二进制请求，Fetch reader 增量解析 gRPC-Web 数据帧；QUESTION、DELTA、SESSION_READY、FEEDBACK、FOLLOW_UP、SUMMARY 与 Qt 使用同一事件契约。最后必须收到 grpc-status trailer 才成功。模型 SSE 仍在 infrastructure/llm/stream.cpp；浏览器链路不再使用 SSE。

首题来自完整题库生成时的增量预览；题库校验成功后发送 SESSION_READY 和最终 QUESTION。报告 MySQL 事务提交成功后才发送完成总结。取消/断开不自动重试回答，刷新后通过历史记录确认最终结果。

## 资源与持久化

- HTTP 使用异步 accept/read，空闲请求有 15 秒读期限，不占业务工作线程。
- 默认 32 个业务工作线程、最多 256 个正在处理/排队的已读请求；超过返回 HTTP 503 + Retry-After。流式发送在业务线程同步写，10 秒发送超时；本轮不是全异步的模型推理实现。
- MySQL 默认 8 个连接，Redis 默认 16 个连接，配置 pool_size 为 1–64。独占借用，最长等待 3 秒；异常连接丢弃，不重放有副作用的操作。
- 报告仓储无进程级串行锁，报告与 outbox 在同一事务写入；缓存失败可回退 MySQL，身份服务失败拒绝访问。
- 会话仍在本实例内存，最多 1000 个。创建时清理无请求持有的已持久化完成会话，以及两小时未访问的会话；最终报告独立保存在 MySQL。重启不能恢复未完成会话。

## 多实例边界

ZooKeeper 后端注册发现和 RabbitMQ 任务继续保留。原来依赖网关的轮询与实例前缀路由已随网关移除；浏览器不会自动从 ZooKeeper 选择后端。当前每个网页入口直接绑定一个实例。多实例统一入口需由部署层提供支持 gRPC-Web 的 HTTP 负载均衡及会话亲和；仅共享数据库不意味着活跃会话可跨实例回答，不能随机重试。

目录详见 PROJECT_STRUCTURE.md，启动详见 ../startup.md，协议详见 GRPC_WEB_API.md。
