# 本地运行与未来 Linux 部署

当前使用原生 C++ 后端与 C++ 网关，没有 Docker、Node.js 服务端运行依赖。

完整构建、数据库自动初始化、Qt/Web 启动和排错见根目录 [startup.md](../startup.md)。账号与数据库说明见 [ACCOUNTS_AND_STORAGE.md](ACCOUNTS_AND_STORAGE.md)。

```bash
cmake -S server -B build/server -DBUILD_CPP_GATEWAY=ON -DCMAKE_BUILD_TYPE=Release
cmake --build build/server --parallel 2
```

两个终端分别执行：

```bash
./build/server/interview_server --config server/config/default_config.json
./build/server/gateway/interview_gateway
```

网页默认 http://127.0.0.1:8888，上游 gRPC 默认 127.0.0.1:50051。

未来 Linux 云服务器可使用相同 CMake 构建（安装 gRPC/Boost/MySQL/hiredis/OpenSSL/Poppler 开发包）。公网入口需提供 HTTPS/WSS 与 gRPC TLS，网关设置 `ALLOWED_ORIGIN=https://实际网页域名`、`COOKIE_SECURE=true`。当前 C++ 网关自身监听明文 HTTP，TLS 由部署入口终止；Qt 使用 `--tls` 连接 gRPC TLS 入口。

运行中的面试仍保存于业务后端内存；最终报告已经写入 MySQL，重启后按账号查询。Redis 登录态需保持可用，数据库和缓存服务不应暴露到公网。网关当前采用有上限的同步工作线程，并非高并发异步 Reactor；上线容量需按负载验证。
