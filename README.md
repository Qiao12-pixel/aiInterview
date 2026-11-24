<<<<<<< HEAD
# AI Interview

C++ 智能面试系统，按使用端和服务进程组织：

```text
client/          Qt 桌面客户端
web/             浏览器前端
server/
  backend/       业务后端：账号、面试、模型、数据库
  gateway/       协议网关：HTTP/WebSocket/SSE → gRPC
  config/        后端私有配置
proto/           跨端通信契约
docs/            目录、架构和接口说明
```

| 目录 | 职责 |
| --- | --- |
| `client/` | Qt 薄客户端，通过原生 gRPC 调用后端 |
| `web/` | 浏览器工作台，WebSocket 会话控制、SSE 反馈/报告、语音与摄像头模块 |
| `server/gateway/` | C++ HTTP/WebSocket/SSE → gRPC 适配和静态资源服务 |
| `server/` | 服务端构建入口与配置 |
| `server/backend/` | 业务后端：账号、面试、模型、PDF、MySQL/Redis |
| `proto/` | Qt、网关、服务端共享的 RPC 契约 |

网页支持浏览器语音转写确认、朗读面试官问题、摄像头预览及用户勾选后的单帧上传；这不是连续 RTC 音视频通话。评分与总结经 JSON 校验后以 SSE 增量展示。

## 本地启动（当前推荐）

依赖安装、C++ 构建、后端/Web/Qt 本地启动命令见 [startup.md](startup.md)。

详细本地构建与运行见 [startup.md](startup.md)，模块职责见 [架构说明](docs/ARCHITECTURE.md) 与 [WebSocket 协议](docs/WEBSOCKET_API.md)。

Qt 和网页共用同一服务端。Qt 已支持原生 gRPC（含 TLS）与 PDF 上传；网页也支持 PDF 简历上传。已加入账号与 MySQL 报告存储；连续 RTC 音视频通话仍待接入。


已加入邮箱注册/登录、Redis 登录态及 MySQL 报告归属和历史查询。配置和初始化见 [账号与存储说明](docs/ACCOUNTS_AND_STORAGE.md)。

网关已统一为 C++17，服务端不需要 Node.js/npm。头文件与实现合并在各模块 src/ 子目录。

网关只负责 HTTP/WebSocket/SSE 与 gRPC 转发；账号、MySQL/Redis、自动建库建表均在业务后端。网关不需要数据库凭据。

目录分层与新增功能落点见 [项目目录说明](docs/PROJECT_STRUCTURE.md)。服务端的 backend 与 gateway 是两个并列独立进程。
