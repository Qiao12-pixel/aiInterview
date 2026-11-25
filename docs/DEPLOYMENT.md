# 本地运行与 Linux 部署边界

构建与启动命令以 [startup.md](../startup.md) 为准。无 Docker、Node.js 服务端或独立网关。

一个 interview_server 进程监听原生 gRPC 50051 与网页/gRPC-Web 8888。Linux 安装对应 gRPC、protobuf、Boost、MySQL、hiredis、OpenSSL、Poppler、RabbitMQ、ZooKeeper 开发包后使用同一 CMake 构建。

监听地址可通过 --listen 和 --web-listen 改为私网地址；--web-root 指向网页目录。生产通过 HTTPS/gRPC TLS 入口终止 TLS，并设置 INTERVIEW_SECURE_COOKIE=1。当前内置 HTTP 接入不直接终止 TLS，不开放跨域 CORS；网页与 gRPC-Web 同源部署。数据库、Redis、MQ、ZooKeeper 保持私网访问。

多实例已内置基于 ZooKeeper 的新会话轮询及回答亲和路由，任一后端可作为业务入口，无需独立网关或部署层会话粘滞。固定入口本身的高可用仍需部署层或客户端备用地址。活跃会话在原实例内存，节点失效不能迁移；已完成报告可从共享 MySQL 查询。ZooKeeper 注册和 MQ worker 见 [CLUSTER.md](CLUSTER.md)。
