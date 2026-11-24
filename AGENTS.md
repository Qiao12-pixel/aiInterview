# 项目协作约定

## 目录边界

- `client/`：Qt 薄客户端，仅 UI、文件上传及 gRPC 传输。禁止实现 LLM 调用、PDF 解析、评分、会话决策或读取模型密钥。
- `server/`：服务端集合与聚合构建。`server/backend/` 为唯一业务后端，`server/gateway/` 为协议转发进程，二者并列；私有配置位于 server/config。不得依赖 Qt UI 类型。
- `web/`：网页前端，只通过服务端公开 API 通信。
- `server/gateway/`：C++ HTTP/WebSocket/SSE 到 gRPC 的协议适配和请求转发，不得实现评分和面试状态流转。
- `proto/`：跨端 gRPC 契约，变更后同步更新网关和客户端。
- `docs/`：架构、API 和迁移设计文档。

## 修改规则

- 新增功能先判断所属模块，避免跨目录直接调用实现类。
- 外部依赖通过接口隔离，业务层不得直接散落 curl、数据库或文件系统调用。
- 配置、日志和错误处理保持统一；不得提交真实 API key、个人路径或简历敏感信息。
- 修改构建文件后至少验证 CMake 配置；涉及 C++ 代码时运行项目已有构建或测试命令。
- 变更目录边界时同步更新 `docs/ARCHITECTURE.md`。

- 账号逻辑集中在 server/backend/src/accounts，通过后端 gRPC 暴露；数据库与 Redis 仅由后端访问。网关不得链接 mysqlclient/hiredis，不读取模型或数据库配置、不初始化数据库。报告写入通过 ports/report_store.h 注入。

- 头文件与实现同处各模块 src/ 子目录，不再新增独立 include/ 或 shared/ 目录。
