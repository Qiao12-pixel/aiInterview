# Qt gRPC 客户端

仅负责界面、RPC 通信、选择并上传简历、展示题目/评分/报告。业务状态与模型配置由服务端维护。客户端不依赖 libcurl、PoDoFo、Poppler、spdlog 或模型 API Key。

```sh
cmake -S . -B build/desktop -DBUILD_PROJECT_TESTS=ON
cmake --build build/desktop --parallel 2
./build/desktop/aiInterview --server 127.0.0.1:50051
```

先在本机启动 C++ 后端。默认无需参数即连接 `127.0.0.1:50051`。也可通过 `AIINTERVIEW_SERVER` 设置地址。

连接云端 TLS gRPC 入口：

```sh
./build/desktop/aiInterview --server grpc.example.com:443 --tls
```

自签或企业 CA 使用 `--ca /path/to/ca.pem`，不关闭证书校验。模型配置 `server/config/default_config.json` 只交给服务端。

`src/transport` 不依赖 Qt，负责协议封装、超时和取消；`src/ui` 只处理展示与用户交互。
