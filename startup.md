# 本地构建与启动

从项目根目录执行。不使用 Docker；只启动一个 C++ 后端进程，直接提供网页和 gRPC-Web，也提供 Qt 使用的原生 gRPC。网页是原生 HTML/CSS/JavaScript，无需 npm 安装或打包。

## 依赖

macOS：

```bash
brew install cmake qt boost grpc protobuf curl nlohmann-json pkg-config poppler mysql redis hiredis rabbitmq-c zookeeper
brew services start mysql
brew services start redis
```

CMake 至少 3.24。Homebrew 包无法定位时设置 CMAKE_PREFIX_PATH 和 PKG_CONFIG_PATH。Linux Ubuntu 对应安装 `cmake g++ libboost-dev libgrpc++-dev protobuf-compiler-grpc libprotobuf-dev libcurl4-openssl-dev nlohmann-json3-dev pkg-config libpoppler-cpp-dev default-libmysqlclient-dev libhiredis-dev libssl-dev librabbitmq-dev libzookeeper-mt-dev`；Qt 另需 Qt6 Widgets/Network/Test 开发包。

RabbitMQ、ZooKeeper 仅在启用相关配置时需要启动，见 docs/CLUSTER.md；编译需要相应客户端库。

## 构建与启动后端 / Web

```bash
cmake -S server -B build/server -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON
cmake --build build/server --parallel 3
ctest --test-dir build/server --output-on-failure
./build/server/interview_server --config server/config/default_config.json \
  --listen 127.0.0.1:50051 --web-listen 127.0.0.1:8888 --web-root web
```

打开 **http://127.0.0.1:8888**。无需运行 interview_gateway；该产物与目录已移除。同一个后端进程读取原有配置文件，现有模型配置不需要迁移。私有配置不提交版本库。

- `--listen`：Qt 原生 gRPC 地址，默认 127.0.0.1:50051。
- `--web-listen`：网页和 gRPC-Web 地址，默认 127.0.0.1:8888。换端口改这里。
- `--web-root`：网页目录，默认 web，相对当前工作目录。
- `--mock-model`：保留数据库/账号，仅用测试模型，不产生模型调用费用。
- `--mock`：匿名、内存模式，仅供隔离测试；网页正常账号流程应使用 --mock-model。

启用 storage 后，C++ 在启动时自动创建缺少的数据库、表和索引。数据库账号需预先存在且具备相应权限，不创建默认应用登录账号。模板见 docs/examples/storage.example.json。连接池可在 storage.mysql.pool_size（默认 8）、storage.redis.pool_size（默认 16）设置，各为 1–64。

网页登录/注册后填写姓名和题量，可上传 PDF 简历（最多 5 MiB、20 页，暂无 OCR），再创建会话。首题、评分、追问、总结通过 gRPC-Web 服务端流增量展示，最终报告保存在 MySQL，历史恢复为对话。支持浏览器语音转写和朗读，已移除视频。页面固定高度，对话区内部滚动。

## Qt 构建与启动

```bash
cmake -S . -B build/desktop -DCMAKE_BUILD_TYPE=Debug -DBUILD_PROJECT_TESTS=ON
cmake --build build/desktop --parallel 2
./build/desktop/aiInterview --server 127.0.0.1:50051 --account-api http://127.0.0.1:8888
```

Qt 原生 gRPC 和账号 HTTP 兼容接口现在均由同一后端提供。云端 TLS 入口可使用 --tls、--ca 和 https 的 --account-api；服务器仍需部署 TLS 终止。

## 测试、修改与排错

```bash
node --test web/tests/*.test.js
cmake -S tools/loadtest -B build/loadtest -DCMAKE_BUILD_TYPE=Release
cmake --build build/loadtest --parallel 2
python3 tools/loadtest/run_upgrade.py \
  --backend ./build/server/interview_server \
  --grpc-bench ./build/loadtest/grpc_loadtest \
  --output docs/benchmarks/local/upgrade-results.json
```

Node 只用于可选网页测试，不是服务端运行依赖。压测使用本机 MySQL root 和 Redis、随机隔离库/前缀，自动清理，不调用付费模型，不停止其他服务。

修改 C++ 后重新构建并重启；修改网页后刷新即可。proto 改动后重建 C++ 并同步 web/src/rpc/protobuf.js。已有 CLion 缓存如还引用 gateway preset 或旧目录，重新加载 CMake。

端口冲突使用 `lsof -nP -iTCP:8888 -sTCP:LISTEN` 检查，或指定新 --web-listen；不要终止未知进程。旧端口出现其他应用时，使用实际打印地址并清理该站点旧缓存。模型 401 检查私有配置 Key；Redis 故障时登录不放行；已完成报告保存在 MySQL，但身份验证仍需要 Redis。

Ctrl+C 停止后端。重启会中断未完成的内存会话，最终报告不丢失。多实例现阶段每个 Web 入口绑定一个实例；原网关轮询已移除，统一负载均衡入口需要部署层会话亲和，详见 docs/CLUSTER.md。
