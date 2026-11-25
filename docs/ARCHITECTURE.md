# 架构与依赖边界

单个 interview_server 进程同时提供原生 gRPC（默认 50051）和网页/gRPC-Web（默认 8888）。不再存在 server/gateway、独立协议代理或 WebSocket。浏览器受 Web API 限制，使用 gRPC-Web，不是原生 HTTP/2 gRPC。

```text
Qt ──原生 gRPC──────────┐
                       api::Service → application/Engine → ports → infrastructure
Web ──gRPC-Web──────────┘                                  ├─ 火山模型（上游 SSE）
     transport/web                                      ├─ MySQL / Redis 连接池
                                                        └─ MySQL outbox → RabbitMQ → 报告任务
```

transport/web 仅处理 HTTP、gRPC-Web framing、Cookie 和静态资源，直接调用同进程 api::Service，不建立本机 gRPC client 连接。启用集群后，服务入口通过统一路由策略选择执行实例；目标是自身则本地执行，否则使用原生 gRPC 转发到目标实例一次。api/native.cpp 将原生 gRPC 上下文适配成相同服务接口。账号模块负责验证、scrypt、登录态和限流，应用层负责出题、追问、评分及报告。

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

ZooKeeper 临时节点发布实例 ID 与原生 gRPC 地址，约每秒读取成员快照；断线清空快照、会话过期重新注册。`infrastructure/cluster/Router` 对新会话按排序后的成员列表轮询，`api/routing` 将请求交给目标实例。Qt 原生 gRPC 和 Web gRPC-Web 复用此策略，客户端可连接任意一个后端入口，无独立网关。

会话 ID 为 instance_id + 随机值，回答固定路由到原实例。跨实例请求携带原始 Authorization，目标仍校验账号和会话归属；内部跳数标记防止递归转发，原生调用 deadline/取消向下游传播。一个业务请求最多发往一个执行实例，禁用内部 gRPC 自动重试；UNAVAILABLE 后该地址短暂冷却 2 秒，注册节点删除后从候选集移除。不存在可用快照时拒绝创建，不使用失效快照猜测目标。

账号、历史和最终报告在入口本地处理，使用共享 MySQL/Redis，不需要随活跃会话路由。正在进行的会话仍在内存，所属节点宕机后不能迁移；已持久化报告可从其他入口查询。每个入口有独立轮询游标，不承诺全局严格等量或按实时 CPU 加权。ZooKeeper 是控制面的成员发现，不承载业务请求。

任意后端都能作为统一业务入口，但固定入口本身宕机时客户端连接仍会失败；公网域名高可用需要部署层 TCP/HTTP 入口或客户端备用地址。部署层无需再实现会话粘滞，因为应用层会根据会话 ID 找到原实例。服务间默认明文 gRPC 仅适用于可信私网，本轮未实现 mTLS。

目录详见 PROJECT_STRUCTURE.md，启动详见 ../startup.md，协议详见 GRPC_WEB_API.md。
