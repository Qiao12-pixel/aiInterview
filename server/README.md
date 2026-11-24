# 服务端

server 是服务端集合，不代表单个进程。两个子项目按运行职责并列：

| 目录 | 用途 | 启动产物 |
| --- | --- | --- |
| `backend/` | 业务后端：账号、面试、LLM、PDF、MySQL/Redis | `interview_server` |
| `gateway/` | 协议网关：HTTP/WebSocket/SSE 与 gRPC 转发 | `interview_gateway` |
| `config/` | 后端私有运行配置，含模型及数据库凭据 | 不单独运行 |

网关不读取 backend 的数据库配置、不连接 MySQL/Redis、不建表。只有业务后端访问数据与外部模型。

## 一起构建

在项目根目录执行：

```bash
cmake -S server -B build/server -DBUILD_CPP_GATEWAY=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build build/server --parallel 2
ctest --test-dir build/server --output-on-failure
```

原有常用启动路径保留：

```bash
./build/server/interview_server --config server/config/default_config.json
# 另一个终端：
./build/server/gateway/interview_gateway
```

## 独立构建

```bash
cmake -S server/backend -B build/backend
cmake --build build/backend --parallel 2
cmake -S server/gateway -B build/gateway
cmake --build build/gateway --parallel 2
```

独立构建时产物分别为 `build/backend/interview_server` 和 `build/gateway/interview_gateway`。

[本地启动](../startup.md) · [项目目录](../docs/PROJECT_STRUCTURE.md) · [后端模块](backend/README.md) · [网关模块](gateway/README.md)
