# AI Interview

C++17 面试后端，Qt 与 Web 共用出题、简历解析、评分和持久化报告能力。浏览器通过后端内置的 gRPC-Web 直接接入，Qt 使用原生 gRPC；单进程提供两种入口，无独立网关。

目录：client/ 桌面 UI，web/ 网页，server/backend/ 业务与传输，server/discovery/ ZooKeeper，proto/ 契约。MySQL 保存账号/报告，Redis 保存登录态/缓存，可选 RabbitMQ outbox 异步报告任务。启用 ZooKeeper 后，后端内置新会话轮询和跨实例回答亲和路由，任一节点均可作为业务入口。

本地构建启动见 [startup.md](startup.md)，模块边界见 [架构](docs/ARCHITECTURE.md)，接口见 [gRPC-Web API](docs/GRPC_WEB_API.md)，升级复测见 [升级复测报告](docs/benchmarks/2026-11-24/UPGRADE.md)。不使用 Docker，网页无需打包。
