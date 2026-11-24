# Qt gRPC 客户端

仅负责界面、RPC 通信、选择并上传简历、展示题目/评分/报告。业务状态与模型配置由服务端维护。客户端不依赖 libcurl、PoDoFo、Poppler、spdlog 或模型 API Key。

```sh
cmake -S . -B build/desktop -DBUILD_PROJECT_TESTS=ON
cmake --build build/desktop --parallel 2
./build/desktop/aiInterview --server 127.0.0.1:50051
```

先在本机启动 C++ 后端。默认连接 `127.0.0.1:50051`，登录窗口通过 `--account-api http://127.0.0.1:8888` 指定账号 API，登录后将 token 在内存中交给 gRPC。也可通过 `AIINTERVIEW_SERVER` 设置地址。

连接云端 TLS gRPC 入口：

```sh
./build/desktop/aiInterview --server grpc.example.com:443 --tls
```

自签或企业 CA 使用 `--ca /path/to/ca.pem`，不关闭证书校验。模型配置 `server/config/default_config.json` 只交给服务端。

`src/transport` 不依赖 Qt，负责协议封装、超时和取消；`src/ui` 只处理展示与用户交互。

Qt 构建规则位于 client/CMakeLists.txt，也支持 `cmake -S client -B build/desktop` 独立配置；项目根目录 CMake 只负责模块聚合选择。
