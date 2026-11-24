# 账号与报告持久化

## 职责

- MySQL `users`：用户 ID、唯一邮箱、昵称、scrypt 密码哈希；候选人姓名不作为用户身份。
- MySQL `interview_reports`：最终 JSON 报告、问答明细、owner_user_id 和生成时间，作为持久化数据源。
- Redis：过期登录态、登录尝试限流、报告缓存。缓存故障时报告回退 MySQL；身份服务故障时不放行匿名访问。
- 进行中的面试会话仍在内存，重启会中断；最终报告不会因进程重启丢失。

## 本地配置与初始化

不使用 Docker 和 Node.js。先运行本机 MySQL/Redis，安装构建所需 libmysqlclient、hiredis 和 OpenSSL。

私有配置 `server/config/default_config.json` 的 storage 模板见 `docs/examples/storage.example.json`。本次本机专用数据库、账号和私有配置已创建，用户的既有模型参数保留。应用登录账号需要通过网页或 Qt 自行注册。

```bash
cmake -S server -B build/server -DBUILD_CPP_GATEWAY=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build build/server --parallel 2
```

服务启动时由 C++ 自动创建配置指定的数据库和应用表，兼容旧报告表并补充所需字段/索引。旧匿名报告保持未归属，已有 scrypt 密码和 Redis 登录态继续兼容。数据库账号须事先存在，首次建库/建表需 CREATE，补字段和索引需 ALTER/INDEX，业务读写需 SELECT/INSERT/UPDATE。程序不会创建数据库账号或授予权限；私有配置不提交版本库。

## 启动与使用

终端一：

```bash
./build/server/interview_server --config server/config/default_config.json
```

终端二：

```bash
./build/server/gateway/interview_gateway
```

网页默认 http://127.0.0.1:8888。点击“登录 / 注册”，完成面试后左侧刷新最近 20 条报告；刷新、退出重登以及两个进程重启后仍可查询本人报告。登录凭据使用 HttpOnly/SameSite Cookie，默认有效期 7 天。

Qt：

```bash
./build/desktop/aiInterview --server 127.0.0.1:50051 --account-api http://127.0.0.1:8888
```

Qt 登录窗口通过 C++ 网关认证，token 仅在进程内存中保存，通过 gRPC Authorization metadata 发送，不在磁盘保存密码。Qt 历史列表 UI 尚未加入，后端 ListReports RPC 已提供；同一账号在 Web 可查询 Qt 产生的最终报告。

不调用模型但保留数据库/账号链路，面试后端可以用 `--mock-model`。旧 `--mock` 会跳过存储和身份，仅用于隔离匿名测试，不要与真实账号网关混用。

## 模块与权限

C++ 后端 `server/backend/src/accounts/service.*` 通过 `repositories.h` 调用 MySQL 用户仓储和 Redis 会话仓储。`crypto.*` 使用 OpenSSL scrypt，兼容 Node 的盐字符串和参数。用户数据库操作使用参数化语句，随机 token 在 Redis 中以 SHA256 摘要为键。

业务后端通过 `src/ports/identity.h` 和 Redis 身份适配器再次验证 metadata；报告 GetReport/ListReports 按 owner_user_id 校验。网关只转发报告 RPC，不读取或评分面试报告。

- Web 账号：`/api/account/register`、`login`、`me`、`logout`
- Qt 账号：`/api/account/native-register`、`native-login`，返回进程内 token
- 历史：`/api/reports`，20 条/页，可指定 offset
- 报告：`/api/report`，本人报告 SSE

公网入口需要 HTTPS/WSS、gRPC TLS，设置 `ALLOWED_ORIGIN=https://实际域名` 和 `COOKIE_SECURE=true`。尚无邮箱验证、密码找回、OAuth、账号删除和跨设备踢出，当前为基础账号模块。

## 验证

```bash
ctest --test-dir build/server --output-on-failure
python3 server/gateway/tests/integration.py \
  --backend ./build/server/interview_server \
  --gateway ./build/server/gateway/interview_gateway \
  --qt ./build/desktop/client_account_smoke
```

脚本依赖 Python 标准库与 mysql、redis-cli，要求本机 root 可连接 MySQL。测试创建并清理随机测试库/Redis 前缀，覆盖注册/重复邮箱、错误密码、旧 Node 哈希兼容、Cookie 恢复、WebSocket、PDF/SSE、本人报告、跨用户拒绝、两个进程重启、退出及空闲 WebSocket 的退出清理。单元测试覆盖数据库写入失败回滚。

账号服务现在完全在 C++ 业务后端。网关无需数据库密码，只需 --grpc-target 指定上游；不依赖 MySQL/Redis 库，也不创建数据库。API 保持不变。
