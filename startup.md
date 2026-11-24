# 本地构建与启动

所有命令从项目根目录执行。服务端和网关均为 C++17 程序，不使用 Docker，不需要 Node.js/npm。网页保持原生 HTML/CSS/JavaScript，由 C++ 网关提供静态资源，无需打包。

## 1. 本机依赖

macOS 首次安装（已有依赖可跳过）：

```bash
xcode-select --install
brew install cmake qt boost grpc protobuf curl nlohmann-json pkg-config poppler mysql redis hiredis
brew services start mysql
brew services start redis
```

Linux Ubuntu 可安装对应的 `cmake g++ libboost-dev libgrpc++-dev protobuf-compiler-grpc libprotobuf-dev libcurl4-openssl-dev nlohmann-json3-dev pkg-config libpoppler-cpp-dev default-libmysqlclient-dev libhiredis-dev libssl-dev`；构建 Qt 时另外安装 Qt6 Widgets/Network/Test 开发包。本机需要先有 MySQL/Redis 服务。

CMake 至少 3.24。macOS 若找不到 Homebrew 包：

```bash
export CMAKE_PREFIX_PATH="$(brew --prefix)${CMAKE_PREFIX_PATH:+:$CMAKE_PREFIX_PATH}"
export PKG_CONFIG_PATH="$(brew --prefix)/lib/pkgconfig${PKG_CONFIG_PATH:+:$PKG_CONFIG_PATH}"
```

当前构建直接使用本机依赖，无需旧 Conan 工具链。

## 2. 构建 C++ 后端与网关

```bash
cmake -S server -B build/server -DCMAKE_BUILD_TYPE=Debug -DBUILD_CPP_GATEWAY=ON -DBUILD_TESTING=ON
cmake --build build/server --parallel 2
ctest --test-dir build/server --output-on-failure
```

可执行文件：

- `build/server/interview_server`：gRPC 面试服务
- `build/server/gateway/interview_gateway`：HTTP/WebSocket/SSE 网关

只构建网关也可以：

```bash
cmake -S server/gateway -B build/gateway -DCMAKE_BUILD_TYPE=Debug
cmake --build build/gateway --parallel 2
```

此时网关可执行文件在 `build/gateway/`。

## 3. 服务端配置与自动建库建表

继续使用 `server/config/default_config.json`，保留已有 `llm` 模型配置和 `storage` 数据库配置。不得把真实 Key 或数据库密码提交到版本库。

本次开发已初始化本机数据库和专用账号，私有配置中已有连接信息。新机器需安装 MySQL/Redis，配置已有数据库账号和 storage.database，字段模板见 `docs/examples/storage.example.json`，详细步骤见 [账号与存储说明](docs/ACCOUNTS_AND_STORAGE.md)。

启用 storage 后，面试后端启动时检查并创建数据库、用户表和报告表，自动补充旧报告表缺少的归属列和索引。已有用户、密码、报告保留，不创建应用登录用户。

## 4. 启动面试后端（终端一）

```bash
./build/server/interview_server --listen 127.0.0.1:50051 --config server/config/default_config.json
```

看到 `Interview gRPC listening on 127.0.0.1:50051` 表示监听成功。

仅替换模型、不产生模型费用，但保留 MySQL/Redis 和账号验证：

```bash
./build/server/interview_server --mock-model --listen 127.0.0.1:50051 --config server/config/default_config.json
```

旧 `--mock` 模式则完全跳过存储和身份校验，仅用于隔离的匿名测试，不要与启用真实账号的网关混用。

## 5. 启动 C++ 网关与 Web（终端二）

```bash
./build/server/gateway/interview_gateway
```

打开 **http://127.0.0.1:8888**，以网关实际打印的 URL 为准。

自定义端口或后端地址：

```bash
./build/server/gateway/interview_gateway \
  --host 127.0.0.1 --port 18080 --grpc-target 127.0.0.1:50051
```

也兼容环境变量：

```bash
PORT=18080 GRPC_TARGET=127.0.0.1:50051 ./build/server/gateway/interview_gateway
```

网关默认参数在 `server/gateway/src/config.h/.cpp`，命令行参数优先于环境变量。静态目录可用 `--web-root /path/to/web` 或 `WEB_ROOT` 设置。应用模型 Key 仅供面试后端使用。

```text
浏览器 → HTTP / WebSocket / SSE → C++ 网关 → gRPC → C++ 面试后端
```

不要再执行 `npm start --prefix server/gateway`，旧 Node 运行时已移除。也不要用普通静态 HTTP 服务单独启动网页，否则没有 `/ws` 和 `/api/`。

## 6. Web 使用

1. 点击“登录 / 注册”，创建自己的账号；不存在预设应用账号。
2. 输入候选人姓名、题目数量，可选择 PDF 简历。
3. 点击“新建会话”，首题通过 SSE 增量展示；题库校验完成后才可回答。
4. 回答评价、追问/下一题、总结通过 SSE 返回。最终报告保存到 MySQL，左侧显示最近历史。

简历最大 5 MiB、20 页，扫描件暂不支持 OCR；不选简历则默认出题。网页本身固定高度，对话区内部滚动。语音转写由浏览器提供，题目可朗读；摄像头可预览并选择上传一张 JPEG。连续 RTC 音视频通话尚未接入。

## 7. 构建与启动 Qt（终端三，可选）

```bash
cmake -S . -B build/desktop -DCMAKE_BUILD_TYPE=Debug -DBUILD_PROJECT_TESTS=ON
cmake --build build/desktop --parallel 2
./build/desktop/aiInterview --server 127.0.0.1:50051 --account-api http://127.0.0.1:8888
```

Qt 登录/注册需要网关提供账号 HTTP 接口，面试业务直接调用 gRPC。默认服务地址分别为 `127.0.0.1:50051` 和 `http://127.0.0.1:8888`。Qt token 仅保存在进程内存。

未来连接云端 TLS 入口：

```bash
./build/desktop/aiInterview --server grpc.example.com:443 --tls --account-api https://interview.example.com
```

可选自定义 CA 使用 `--ca /path/to/ca.pem`；本机默认 gRPC 为明文，不添加 `--tls`。

CLion 可以选择根目录的 `desktop-debug`、`backend-debug`、`gateway-debug` preset。若旧缓存引用已删除 include 路径或 Conan 工具链，重置缓存再重新加载。头文件和实现现在同处 `client/src/`、`server/backend/src/` 和 `server/gateway/src/` 的模块子目录。

## 8. 日常修改与停止

| 修改内容 | 操作 |
| --- | --- |
| 后端/网关 C++ | `cmake --build build/server --parallel 2` 后重启相应进程 |
| Qt C++ | 重新构建 `build/desktop` 后重启 Qt |
| `proto/` | 重新构建后端、网关和 Qt，并重启 |
| `web/` | 刷新浏览器，无需编译 |
| 配置文件 | 重启读取该配置的后端 |

对应终端按 Ctrl+C 退出，Qt 关闭窗口。进行中的会话保存在内存，重启会中断；最终报告已经存入 MySQL，不会因进程重启丢失。Redis 用于登录态和报告缓存。

## 9. 测试与排错

C++ 单元测试：

```bash
ctest --test-dir build/server --output-on-failure
```

全链路测试（Python 标准库、本机 root 可访问 MySQL、Redis 运行中；只创建并清理随机测试库/Redis 前缀）：

```bash
python3 server/gateway/tests/integration.py \
  --backend ./build/server/interview_server \
  --gateway ./build/server/gateway/interview_gateway \
  --qt ./build/desktop/client_account_smoke
```

- 端口占用：`lsof -nP -iTCP:8888 -sTCP:LISTEN`，后端端口为 50051。先停止旧服务或改端口。
- `localhost` 指向其他 IPv6 服务：使用网关打印的 `127.0.0.1` URL。
- 旧端口显示 llama-ui：可能是旧 Service Worker；换端口或清理该站点缓存。
- gRPC UNAVAILABLE：检查后端监听地址及网关 `--grpc-target`。
- 模型 HTTP 401：更新私有配置中的有效 Key 并重启面试后端。
- Redis 不可用：登录会被拒绝；报告缓存故障可回退 MySQL，但账号身份仍需要 Redis。

启动时若提示 MySQL 自动初始化失败，请确认数据库账号存在，且对 storage.mysql.database 指定的库具备 CREATE、ALTER、INDEX 及业务读写权限。只自动创建缺少的库、表、归属字段和索引，不删除已有数据；不需要单独建表命令。

目录分层已按进程整理：`client/`、`web/`、`server/backend/`、`server/gateway/` 和 `proto/`。详细说明见 [PROJECT_STRUCTURE.md](docs/PROJECT_STRUCTURE.md)。原有聚合启动路径保留；如需独立构建业务后端，可用 `cmake -S server/backend -B build/backend`；Qt 可独立 `cmake -S client -B build/desktop`。
