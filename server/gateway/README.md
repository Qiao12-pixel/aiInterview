# C++ 网关

独立 C++17 服务，使用 Boost.Beast 提供网页 HTTP、WebSocket 和 SSE，以原生 gRPC 调用面试后端。不再使用 Node.js 或 npm。外部 API 和旧客户端消息格式保持兼容。

## 构建

```bash
cmake -S server/gateway -B build/gateway -DCMAKE_BUILD_TYPE=Debug
cmake --build build/gateway --parallel 2
ctest --test-dir build/gateway --output-on-failure
```

依赖：CMake 3.24+、C++17、Boost headers、gRPC/Protobuf、OpenSSL、nlohmann_json。

## 启动与自动初始化

```bash
./build/gateway/interview_gateway
```

默认网页地址 `http://127.0.0.1:8888`，gRPC 上游 `127.0.0.1:50051`。模型 Key 只由业务后端使用，网关不读取 storage 配置。已有 MySQL 用户与 scrypt 密码、Redis 会话格式不变。

参数：`--config`、`--host`、`--port`、`--grpc-target`、`--web-root`、`--allowed-origin`。兼容环境变量 `GATEWAY_CONFIG`、`HOST`、`PORT`、`GRPC_TARGET`、`WEB_ROOT`、`ALLOWED_ORIGIN`、`COOKIE_SECURE`。参数优先于环境变量。

## 源码职责（头文件与实现放在同一目录）

- `src/main.cpp`、`config.*`：进程启动、参数和配置
- `src/transport/server.*`：监听、连接并发上限、退出清理
- `src/transport/http.*`、`websocket.*`、`sse.*`：协议处理
- `src/transport/grpc_client.*`：后端 RPC 封装
- `src/routes/`：账号、创建面试/回答、报告、静态文件接口适配
- `src/routes/accounts.cpp`：账号 HTTP 请求与后端 Account gRPC 的适配、Cookie 响应
- `src/common/`：JSON、错误类型、上传编码校验

网关不实现面试出题、评分或状态推进。并发连接由工作线程处理，最多 128 个连接；阻塞 RPC 有 deadline，SSE 有 15 秒心跳及断线取消，退出时关闭连接并回收线程。当前不是高并发异步 Reactor 实现，生产扩容前需结合负载做并发测试。

## 验证

```bash
python3 server/gateway/tests/integration.py \
  --backend ./build/server/interview_server \
  --gateway ./build/gateway/interview_gateway
```

集成测试需要本机 root 可连接 MySQL 和运行中的 Redis；创建并清理独立随机测试库及 Redis 前缀。可添加 `--qt ./build/desktop/client_account_smoke` 验证 Qt 登录兼容。测试脚本只依赖 Python 标准库及 mysql、redis-cli。

网关不连接 MySQL/Redis，不创建数据库，启动不需要数据库账号。注册、密码哈希、登录态、限流和报告权限均通过 gRPC 交给后端。
