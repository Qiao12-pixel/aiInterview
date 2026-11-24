# Linux 部署与开发

## 当前可运行链路

浏览器页面由网关提供，同源 WebSocket `/ws` 转发到 C++ gRPC 服务。模型 Key 仅在 C++ 后端读取，浏览器和网关不接触模型 Key。Qt 已接入相同的原生 gRPC 服务，仅负责展示与文件上传。

## 本地运行

环境需具备 CMake、C++17、gRPC/Protobuf、libcurl、nlohmann_json 和 poppler-cpp（PDF 解析）。macOS 可使用已安装的 Homebrew 依赖。

```sh
cmake -S server -B build/server -DCMAKE_BUILD_TYPE=Release
cmake --build build/server --parallel 2
ctest --test-dir build/server --output-on-failure
./build/server/interview_server --config server/config/default_config.json
```

另一个终端启动网关：

```sh
npm ci --prefix server/gateway
npm start --prefix server/gateway
```

打开 http://127.0.0.1:8080，填写姓名和题目数量，创建会话、提交回答，完成后查看报告。网页必须从网关端口打开，单独的静态 HTTP 服务无法提供 `/ws`。

无模型费用的开发验证：服务端增加 `--mock` 参数，界面会收到明确的测试题和测试评分。真实模型默认读取指定 JSON 中的 `llm` 字段，与 Qt 配置格式一致。

## 网关环境变量

- `GRPC_TARGET`：后端地址，默认 `127.0.0.1:50051`
- `HOST` / `PORT`：网关监听地址，默认 `127.0.0.1:8080`
- `ALLOWED_ORIGIN`：WebSocket 允许的网页 Origin，默认当前 HTTP Host

`/healthz` 仅表示网关存活，不代表模型和 gRPC 上游已就绪。

## 本阶段限制

- 会话保存在内存，重启会丢失；最多保存 1000 个会话，后续需加入过期清理和持久化。
- 单会话串行处理；模型请求在 gRPC worker 中执行，有超时，尚未拆为持久化任务队列。
- Qt 支持上传 PDF（5 MiB/20 页上限）并在后端解析；网页暂未加入上传控件。
- 浏览器语音转写由 Web Speech API 提供，候选人确认文本后通过 SSE 提交；TTS 使用浏览器 Speech Synthesis。视频可本地预览，只有用户勾选后才把单张 JPEG 帧随回答传给模型。原始音频和视频不会持续上传或保存；这不是实时 RTC 音视频通话。gRPC 连续音频/视频流和火山 RTC/ASR/TTS 接入尚未完成。
- `/api/answer` 与 `/api/report` 使用经过 Origin 校验的 POST + SSE；事件包括状态、反馈/总结增量、最终业务事件和终止帧。模型输出启用 JSON 模式和字段校验；异常结构会重试一次，仍无效则返回错误，不会把原始模型 JSON 显示在对话里。
- 网关只允许当前连接操作其创建的会话；断线后需创建新会话，尚无账号、鉴权和恢复协议。
- 当前按本机手动启动 C++ 服务和 Node 网关运行。未来接入云服务器前，需完成用户鉴权、TLS、持久化和请求幂等。

WebSocket 消息格式见 `docs/WEBSOCKET_API.md`。端到端测试在启动 `--mock --listen 127.0.0.1:50061` 服务和 `GRPC_TARGET=127.0.0.1:50061 PORT=8081` 网关后运行 `npm test --prefix server/gateway`。

## Qt 远程客户端

根目录客户端构建不使用旧的 Conan 工具链；若旧 CLion 缓存仍引用 Conan，重置 CMake 缓存后重新加载。可使用根目录 `desktop-debug` preset，或：

```sh
cmake -S . -B build/desktop -DBUILD_PROJECT_TESTS=ON
cmake --build build/desktop --parallel 2
./build/desktop/aiInterview --server 127.0.0.1:50051
# 云端经 TLS 反向代理后：
./build/desktop/aiInterview --server grpc.example.com:443 --tls
```

Qt 不再读取模型 JSON；启动参数仅指定 gRPC 地址和 TLS/CA。PDF 不在 Qt 解析，由上传字节在服务端提取；扫描件尚不支持 OCR。
