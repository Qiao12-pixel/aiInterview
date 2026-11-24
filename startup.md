# 本地构建与启动

以下命令均从项目根目录执行，适用于当前 macOS 开发环境。CMake 最低版本为 3.24，C++ 标准为 C++17。

## 1. 安装依赖（首次）

已有依赖可以跳过。macOS 使用 Homebrew：

```bash
xcode-select --install
brew install cmake qt grpc protobuf curl nlohmann-json pkg-config poppler node
```

- Qt 客户端依赖 Qt6、gRPC 和 Protobuf。
- C++ 后端依赖 gRPC、Protobuf、libcurl、nlohmann_json 和 Poppler C++。
- WebSocket 网关依赖 Node.js 和 npm；建议 Node.js 22 或更新的受支持版本。
- 当前构建直接查找本机依赖，不需要运行 Conan，也不使用旧 Conan 工具链。

若 CMake 找不到 Homebrew 包，可在当前终端设置：

```bash
export CMAKE_PREFIX_PATH="$(brew --prefix)${CMAKE_PREFIX_PATH:+:$CMAKE_PREFIX_PATH}"
export PKG_CONFIG_PATH="$(brew --prefix)/lib/pkgconfig${PKG_CONFIG_PATH:+:$PKG_CONFIG_PATH}"
```

## 2. 配置模型（仅后端读取）

后端使用 `server/config/default_config.json` 中的 `llm` 配置，启动前确认以下字段已填写：

```json
{
  "llm": {
    "api_url": "https://ark.cn-beijing.volces.com/api/v3/chat/completions",
    "api_key": "填写有效的火山方舟 Key",
    "model": "填写已开通的模型 ID 或推理接入点 ID",
    "thinking_type": "disabled",
    "temperature": 0.3,
    "max_tokens": 4096,
    "timeout_seconds": 120
  }
}
```

这是字段示例；已配置的文件无需覆盖。Qt、浏览器和网关均不读取模型 Key。模型配置修改后重启后端生效。

## 3. 编译 C++ 后端

```bash
cmake -S server -B build/server -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON
cmake --build build/server --parallel 2
```

可执行文件：`build/server/interview_server`。该工程独立构建，不需要 Qt。

运行单元测试：

```bash
ctest --test-dir build/server --output-on-failure
```

## 4. 启动后端（终端一）

在项目根目录运行并保持终端开启：

```bash
./build/server/interview_server \
  --listen 127.0.0.1:50051 \
  --config server/config/default_config.json
```

看到 `Interview gRPC listening on 127.0.0.1:50051` 表示监听成功。

不调用真实模型、仅验证交互流程时，改用以下命令，两种模式不要同时占用同一端口：

```bash
./build/server/interview_server --mock --listen 127.0.0.1:50051
```

`--mock` 不读取模型配置，也不产生模型调用费用；返回的是测试题和测试评分，PDF 仍在后端真实解析。

## 5. Web 端安装与启动（终端二）

首次运行或锁文件更新后安装网关依赖：

```bash
npm ci --prefix server/gateway
```

启动：

```bash
npm start --prefix server/gateway
```

打开 **http://127.0.0.1:8080**。

当前网页是原生 HTML/CSS/JavaScript，无需单独执行前端编译或打包。网关同时提供网页静态资源和 `/ws` 接口：

```text
浏览器 :8080 → WebSocket 网关 → C++ gRPC 后端 :50051 → 火山模型
```

不要使用 `python -m http.server` 单独启动网页，否则没有 WebSocket 网关，无法进行真实对话。

网页保持固定视口，页面本身不显示滚动条，只有对话列表内部滚动。媒体辅助能力包括浏览器语音转写并确认提交、浏览器朗读题目、摄像头本地预览，以及显式勾选后上传单张 JPEG 画面。评价和总结通过 SSE 增量显示。此版本不是连续 RTC 音视频通话，原始媒体不会持续上传或保存。

如果后端端口不同，可显式指定网关上游和监听端口：

```bash
GRPC_TARGET=127.0.0.1:50052 HOST=127.0.0.1 PORT=8082 npm start --prefix server/gateway
```

此时网页地址为 `http://127.0.0.1:8082`，并应先让后端监听 `50052`。

## 6. 编译并启动 Qt 客户端（终端三，可选）

```bash
cmake -S . -B build/desktop -DCMAKE_BUILD_TYPE=Debug -DBUILD_DESKTOP_CLIENT=ON -DBUILD_BACKEND=OFF
cmake --build build/desktop --parallel 2
./build/desktop/aiInterview --server 127.0.0.1:50051
```

Qt 直接调用 C++ gRPC 后端，**只用 Qt 时无需启动 WebSocket 网关**。Qt 和网页可以同时连接同一后端，各自创建独立会话。

Qt 的可选启动参数：

```bash
# 不指定地址时，默认连接 127.0.0.1:50051
./build/desktop/aiInterview

# 为后续云端 TLS 入口指定地址
./build/desktop/aiInterview --server grpc.example.com:443 --tls

# 使用自定义 CA（必须同时开启 TLS）
./build/desktop/aiInterview --server grpc.example.com:443 --tls --ca /path/to/ca.pem
```

本地后端当前使用明文 gRPC，不要对本地默认连接添加 `--tls`。

## 7. CLion 构建

根目录 `CMakeLists.txt` 默认生成 Qt 客户端目标 `aiInterview`。CLion 中选择该目标，先启动后端，再运行客户端。

也可从终端构建现有 CLion 目录：

```bash
cmake -S . -B cmake-build-debug -DCMAKE_BUILD_TYPE=Debug
cmake --build cmake-build-debug --parallel 2
./cmake-build-debug/aiInterview --server 127.0.0.1:50051
```

如果旧缓存仍引用 Conan 或旧目录，先在 CLion 重置 CMake 缓存并重新加载；专用的 `build/desktop` 与 `build/server` 目录可避免旧缓存干扰。

## 8. 日常修改后重新构建

| 修改内容 | 操作 |
| --- | --- |
| 服务端 C++ | `cmake --build build/server --parallel 2`，然后重启后端 |
| Qt C++ | `cmake --build build/desktop --parallel 2`，然后重启 Qt |
| `proto/interview.proto` | 重新构建服务端和 Qt；重启后端及网关 |
| `server/gateway/src/` | 重启 `npm start --prefix server/gateway` |
| `web/` | 刷新浏览器；有缓存时强制刷新 |
| 模型 JSON 配置 | 重启后端 |

网关和浏览器模块回归测试（需先启动 mock 后端与 8081 网关）：

```bash
npm test --prefix server/gateway
```

## 9. 停止服务与排错

后端和网关均为前台进程，在对应终端按 `Ctrl+C` 停止；Qt 关闭窗口即可。后端会话目前保存在内存，重启后会丢失。

macOS 查看端口占用：

```bash
lsof -nP -iTCP:50051 -sTCP:LISTEN
lsof -nP -iTCP:8080 -sTCP:LISTEN
```

- 后端监听失败或网关报 `EADDRINUSE`：对应端口已被旧服务占用，先停止旧服务，或同时调整后端、网关/Qt 的地址。
- Qt 报 gRPC `UNAVAILABLE`：检查后端是否启动、`--server` 地址是否正确。
- 网页无法连接：从网关的 `8080` 端口访问，检查网关终端日志及 `GRPC_TARGET`。
- `localhost:8080` 显示 Jetty/Java 的 404，但网关启动正常：本机可能同时存在 IPv6 的 Java 服务和 IPv4 的 Node 网关。直接打开 `http://127.0.0.1:8080`；无需停止其他 Java 服务。网关默认只监听 IPv4，`localhost` 可能解析为 IPv6 的 `::1`。
- `127.0.0.1:8080` 显示以前的 llama-ui 等页面，但网关返回的 HTML 标题是 `AI Interview`：可能是该地址遗留的 Service Worker/PWA 缓存。可先用无痕窗口验证；或停止当前网关，运行 `PORT=18080 npm start --prefix server/gateway`，打开 `http://127.0.0.1:18080`。不同端口属于不同 Origin，不会复用旧站点的 Service Worker。若要继续使用旧端口，在浏览器开发者工具 Application → Service Workers 中注销对应 Worker，再清理该站点缓存。
- 模型返回 401：更新服务端配置中的 Key 后重启后端。
- 无模型凭据时：先用 `--mock` 验证前后端通信。

当前 Qt 支持 PDF 上传；网页暂未提供简历上传控件。网页语音转写后确认提交、面试官问题可朗读；视频支持本地预览和勾选上传单张 JPEG。回答评价和总结以 SSE 增量展示。当前不是连续 RTC 音视频通话，账号鉴权与持久化也尚未完成。
