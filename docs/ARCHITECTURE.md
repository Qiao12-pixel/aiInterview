# 项目架构

## 目录职责

- `client/src/`：Qt UI、账号登录窗口、文件选择和原生 gRPC 客户端。
- `web/`：浏览器 HTML/CSS/JS，不包含服务端能力。
- `server/backend/src/`：面试业务、模型调用、PDF 解析、报告存储和 gRPC API。
- `server/gateway/src/`：独立 C++ HTTP/WebSocket/SSE 网关与 gRPC 协议适配。
- `proto/`：所有调用方共同使用的 gRPC 契约及 CMake 生成目标。
- `docs/examples/`：配置示例；仅供阅读，不作为运行时依赖。
- `docs/`：架构和 API 说明。

头文件与实现文件放在同一模块目录，不再分别维护 include/src，也没有 shared 目录。例如 `client/src/ui/MainWindow.h/.cpp`、`server/backend/src/application/engine.h/.cpp`、`server/backend/src/accounts/service.h/.cpp`。

## 运行链路

```text
Web → HTTP / WebSocket / SSE → C++ Gateway → gRPC → C++ 面试服务 → 火山模型
Qt  → 账号 HTTP API ──────────────┘          ↑
Qt  → 原生 gRPC ───────────────────────────┘
```

后端与网关可分别构建运行。Qt 不读取模型 Key；网关不读取 storage 数据配置，不调用 LLM、不解析 PDF、不做评分和面试状态流转。

## C++ 面试服务

以下 src/ 路径均相对于 server/backend/。

- `src/api/main.cpp`：启动和依赖组装。
- `src/api/service.*`、`start_interview.cpp`：RPC 接入、Redis 身份与报告归属验证。
- `src/application/engine.*`、`answer.cpp`、`report.cpp`：创建会话、评估、追问、报告提交与读取。
- `src/domain/`：会话与事件类型。
- `src/ports/`：模型、报告仓储、身份解析接口；业务层不链接具体数据库、CURL 或 gRPC 实现。
- `src/infrastructure/llm/`：模型配置、HTTP、SSE 分片、提示词和结构校验。
- `src/infrastructure/pdf_parser.*`：Poppler 解析上传字节，不保存原始简历。
- `src/infrastructure/storage/`：MySQL 报告仓储、Redis 缓存与身份查询。
- `src/common/`：JSON 前缀和 Base64 工具。

CMake 中的 domain/application/infrastructure/storage 目标隔离依赖。最终报告写入 MySQL 成功才提交完成状态，数据库失败不把会话标记为完成。进行中的会话仍在内存，暂不支持跨实例恢复或幂等重试。

## C++ 网关

以下 src/ 路径均相对于 server/gateway/。

Boost.Beast 处理 HTTP/WebSocket，gRPC C++ stub 调用后端。源码按 config、transport、routes、accounts、common 分开：

- `transport/server.*`：最多 128 个连接、工作线程、连接关闭与退出回收。
- `transport/http.*`、`websocket.*`、`sse.*`：协议处理；SSE 提供 15 秒心跳、同步背压和断线取消。
- `transport/grpc_client.*`：RPC 调用、deadline、状态码映射。
- `routes/`：账号 API、面试创建/回答、历史报告、静态文件。
- 账号注册、密码校验、登录态与限流已迁至后端 `server/backend/src/accounts/`；网关仅调用 Account gRPC。
- 后端 `server/backend/src/accounts/crypto.*`：OpenSSL scrypt、登录 token 和摘要；网关仅生成匿名测试模式的协议 token。

匿名开发模式保持原 session_token 契约，启用账号时由 Redis token 确认身份。网关转发认证请求，由 C++ 面试服务统一验证，不信任浏览器传入的用户 ID。静态目录校验路径和符号链接，不暴露配置文件。

## 首题及回答流

`/api/sessions` 按 Accept 头返回 JSON 或 SSE。SSE 调用 StartInterview RPC，将模型首个 question 的可展示前缀转换为 DELTA；题库校验完成才发 session/QUESTION/done。首题预览未完成时禁止回答，失败则标记未完成。

`/api/answer` 调用 SendText，反馈和总结的可展示字段逐步输出，完整 JSON 校验后才提交业务状态。后续普通题已提前生成，直接通过 SSE 下发。SSE 连接断开不会自动重放回答。

## 账号与数据

MySQL 保存用户、scrypt 哈希和按用户归属的最终报告。Redis 保存过期登录态、限流和可失效报告缓存。网页凭据使用 HttpOnly Cookie；Qt 仅在内存保存 token 并通过 gRPC metadata 传递。旧 Node 密码和会话兼容，无需用户重新注册。

当前没有邮箱验证、密码找回、OAuth、账号删除及完整 RTC 音视频流。语音由浏览器识别后确认提交，摄像头提供预览与可选单张 JPEG；不保存原始媒体。

## 历史会话展示

持久化报告中的 records 是历史对话的数据源。Web report-model.js 兼容当前 evaluation 字段与旧 score/feedback 字段；report-view.js 安全渲染题目、回答、评价及总结卡片，使用 textContent，不暴露 owner/session ID，不把 JSON 直接展示给用户。历史视图只读，与当前面试状态隔离；切换回当前面试恢复原气泡和草稿，不触发模型请求。历史记录不包含未保存的语音、视频或流式草稿，不会据此伪造原始对话。

## 数据库启动初始化

`server/backend/src/infrastructure/database/bootstrap.h/.cpp` 内置数据库和 users/interview_reports 建表 SQL。仅业务后端打开报告/账号仓储时调用 Initialize 接口；CMake 独立目标避免复制初始化逻辑。使用 MySQL 命名锁串行化同库启动，information_schema 检查后补旧归属列与索引，不删除已有数据。数据库名限制为字母数字下划线，账号和密码来自私有配置。独立初始化工具和重复 SQL 文件已移除；建表结构统一维护在 bootstrap.cpp 中。存储配置示例位于 docs/examples/storage.example.json。

## 网关职责收敛

网关只保留浏览器 HTTP/WebSocket/SSE 与 gRPC 的适配，静态网页仍由网关提供。Account RPC 负责 register/login/me/logout，身份和 Cookie/token 只经协议转发。用户仓储、scrypt、Redis 会话/限流在 server/backend/src/accounts，报告/建库在后端 infrastructure。网关 CMake 不链接 MySQL/hiredis 或数据库初始化模块，可以没有数据库配置启动；上游后端不可用时账号和业务请求返回 503。
