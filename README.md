<<<<<<< HEAD
# AI Interview

C++ 智能面试系统，按桌面端、网页和解耦的服务端模块分块。

| 目录 | 职责 |
| --- | --- |
| `client/` | Qt 薄客户端，通过原生 gRPC 调用后端 |
| `web/` | 浏览器工作台，WebSocket 会话控制、SSE 反馈/报告、语音与摄像头模块 |
| `server/gateway/` | Node.js WebSocket/SSE → gRPC 适配和静态资源服务 |
| `server/` | 独立 C++17 gRPC 服务，出题、评分、追问、总结、PDF 解析 |
| `proto/` | Qt、网关、服务端共享的 RPC 契约 |

网页支持浏览器语音转写确认、朗读面试官问题、摄像头预览及用户勾选后的单帧上传；这不是连续 RTC 音视频通话。评分与总结经 JSON 校验后以 SSE 增量展示。

## 本地启动（当前推荐）

依赖安装、C++ 构建、后端/Web/Qt 本地启动命令见 [startup.md](startup.md)。

详细本地构建与运行见 [startup.md](startup.md)，模块职责见 [架构说明](docs/ARCHITECTURE.md) 与 [WebSocket 协议](docs/WEBSOCKET_API.md)。

Qt 和网页共用同一服务端。Qt 已支持原生 gRPC（含 TLS）与 PDF 上传；网页尚无简历上传控件。账号、持久化及音视频交互尚未实现。

