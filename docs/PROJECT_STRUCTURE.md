# 目录职责

```text
client/src/                 Qt UI、账号窗口、原生 gRPC transport
web/                       网页，无需打包
  src/rpc/                 protobuf、gRPC-Web、面试接口
  src/                     会话、历史、账号、简历和语音 UI
server/
  backend/src/
    api/                   统一服务接口与原生 gRPC 适配
    transport/web/         server/http/routes/accounts/rpc/static_files 分开实现
    application/           面试流程、回答、报告
    domain/                会话领域数据
    ports/                 模型、身份、报告存储接口
    accounts/              注册、登录、密码及账号仓储
    infrastructure/
      llm/                 火山模型 HTTP / 上游 SSE
      database/            C++ 自动建库建表
      storage/             MySQL/Redis 连接池、报告及缓存
      messaging/           RabbitMQ、报告文档
    worker/                outbox 发布与异步报告任务入口
  discovery/               ZooKeeper 注册发现
  config/                  私有配置
proto/                     公共 protobuf/gRPC 契约
tools/loadtest/            压测客户端、隔离运行脚本
docs/                     架构、API、原始压测结果
startup.md                 本机依赖、构建和启动
```

头文件与实现同处模块目录。服务端不依赖 Qt；浏览器与 Qt 共用同一业务服务。server/gateway 已删除，未把原来的转发进程简单重命名。
