# 项目架构

## 当前边界

```text
web/ ── WebSocket ── server/gateway/ ── gRPC ── server/
client/ ───────────── 原生 gRPC ──────────┘
                          proto/（共同契约）
```

- `client/`：Qt 界面、文件选择与上传、RPC 客户端、界面状态展示。无 LLM、PDF 解析和面试决策逻辑，不读取模型配置。
- `web/`：浏览器页面和 WebSocket 客户端。
- `server/gateway/`：提供网页静态资源，将 WebSocket JSON 转为 gRPC 请求，隔离浏览器与服务端协议。
- `server/`：唯一的业务实现，包括出题、评分、追问、会话状态机、报告、PDF 解析和模型调用。
- `proto/`：Protocol Buffers 契约及公共 CMake 生成目标，两端不相互引用源文件。

## 服务端内部

- `src/api/main.cpp`、`src/api/service.cpp`：gRPC 服务启动、RPC 端点及输入转换。
- `src/application/engine.cpp`：统一业务编排、会话状态机、最多一次追问、问答记录和最终报告。
- `src/application/answer.cpp`：回答评估、会话事务提交和反馈/总结增量事件。
- `src/infrastructure/llm/config.cpp`、`http.cpp`、`stream.cpp`：模型配置、HTTP 资源管理和 SSE 分片解析。
- `src/common/json_output.cpp`、`infrastructure/llm/contract.cpp`：JSON/Unicode 前缀解码、提示词及模型返回结构校验。
- `src/infrastructure/pdf_parser.cpp`：基于 Poppler C++ 从上传字节提取文本，不读取客户端本地路径，不落盘。

旧 `client/src/interview`、`client/src/services` 的职责已合并到上述模块，不保留第二套本地业务实现。PDF 适配器替换原 PoDoFo 实现，避免 Qt 或 Linux 端绑定某个 Homebrew 版本。

## Qt 调用

Qt 只连接 `host:port`。支持 `--server`、`--tls` 和可选 `--ca`；默认连接本地 `127.0.0.1:50051`。

创建会话可发送最多 5 MiB 的 PDF 内容，服务端限制最多 20 页、100000 字节提取文本。RPC 在后台线程执行，回调主线程更新界面。关闭窗口会取消 RPC。提交回答后由后端返回评分、追问/下一题或总结，客户端不自行推进题目索引。

## 部署边界

后端可独立 `cmake -S server` 构建，无 Qt 依赖。模型 Key 仅在服务端配置文件中，网关和 Qt 不读取它。当前采用本地原生进程运行，启动命令见根目录 `startup.md`。未来云端入口使用 HTTPS/WSS 和 gRPC TLS。

当前会话仍在内存，无用户账号、断线恢复与幂等重试。网页语音通过浏览器识别后确认提交，问题使用浏览器朗读；视频可预览，勾选后随回答上传单张 JPEG，不保存原始媒体。这是语音和单帧画面辅助，不是连续实时音视频通话；完整 RTC/ASR/TTS 流仍待接入。SSE 断流不会自动重放回答。

## 服务端模块解耦

所有服务端进程与配置均位于 `server/`，协议契约 `proto/` 由 Qt 和后端共同使用，不包含业务实现。

- CMake `interview_domain`：会话数据类型与公共 JSON 类型，无网络依赖。
- `interview_application`：会话状态机，只依赖领域类型和 `ports/model.h` 回调接口，不链接 CURL、Poppler 或 gRPC。
- `interview_infrastructure`：实现 LLM 和 PDF 适配器，不依赖应用层；由 API 入口组装。
- `server/gateway/src/config.js`：独立配置；`grpc-client.js`：后端适配器；`http.js`：静态资源和 API 路由；`websocket.js` 与 `rpc/`：WebSocket 请求适配；`answer-route.js`、`report-route.js`：单独的 SSE 端点；`sse-writer.js`、`session-auth.js`、`request-body.js`：SSE 生命周期、会话凭据和请求限额。
- 网关通过 `WEB_ROOT` 指定静态资源目录，开发时默认定位 `web/`；通过 `PROTO_PATH` 可指定协议文件，无需引用 Web 源码或 C++ 实现。

根目录不再设置 shared 模块；旧未引用工具已移除，避免留下第二套配置、日志和状态实现。
