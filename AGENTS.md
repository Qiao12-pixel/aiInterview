# 项目协作约定

## 目录边界

- `client/`：Qt 薄客户端，仅 UI、文件上传及 RPC 传输。不得实现 LLM、PDF 解析、评分、会话决策或读取模型密钥。
- `web/`：HTML/CSS/JavaScript 网页，通过 gRPC-Web 直接调用业务后端；不访问数据库和模型。
- `server/backend/`：唯一业务后端。`api/` 暴露原生 gRPC 与可复用服务接口；`transport/web/` 处理 gRPC-Web 帧、Cookie 和静态文件；`application/` 决定面试流程；`ports/` 隔离外部能力；`infrastructure/` 实现存储、模型、PDF、MQ。不得依赖 Qt UI。
- 不再创建独立 gateway 进程、WebSocket 转发或浏览器 SSE 适配层。模型上游 SSE 位于 infrastructure/llm/stream.cpp。
- `server/discovery/`：ZooKeeper 注册发现基础设施。浏览器不直接访问 ZooKeeper。
- `server/config/`：私有配置，禁止提交真实 Key、数据库密码、个人路径和简历。
- `proto/`：跨端契约，修改后同步 C++ 和 web/src/rpc/protobuf.js 的字段映射与测试。
- `docs/`：架构、API 和启动文档。目录边界改变时同步 docs/ARCHITECTURE.md。

## 修改规则

- 头文件与实现同处模块 src/ 子目录，不新增 include/ 或 shared/。
- 新增能力先确定模块；应用层通过端口依赖外部能力，不散落 curl、数据库和文件系统操作。
- 账号和报告仅由后端访问 MySQL/Redis，报告事务写入经 ports/report_store.h 注入。RabbitMQ/outbox/报告任务属于后端。
- 网络空闲连接不独占业务线程；连接池与工作队列必须有上限；不得自动重放回答等有副作用的请求。
- 配置、日志、错误处理统一且脱敏；不打印登录 token、简历全文及完整回答。
- 修改 CMake 后验证配置；涉及 C++ 运行已有构建和相应测试。
