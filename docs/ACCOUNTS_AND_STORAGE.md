# 账号与报告持久化

MySQL users 保存用户 ID、唯一邮箱、昵称和 scrypt 哈希；interview_reports 保存最终报告、问答和 owner_user_id。Redis 保存有 TTL 的登录态、尝试限流和报告缓存。候选人姓名不作为身份。进行中的会话仍在内存，重启会中断；最终报告不会因重启丢失。

私有配置沿用 server/config/default_config.json，模板见 examples/storage.example.json。启动 C++ 后端时自动补缺失库、表、归属字段和索引，不删除既有数据，不创建默认登录用户。数据库账号需预先存在，首次初始化具备 CREATE/ALTER/INDEX，日常具备 SELECT/INSERT/UPDATE；命令见 ../startup.md。

MySQL 与 Redis 默认连接池大小分别为 8、16，可通过 pool_size 调整到 1–64。借用连接由单请求独占，等待最长 3 秒；连接失败不自动重放写入。MySQL 报告与 outbox 同事务提交，报告仓储不再用全局锁串行保存。Redis 缓存失败可回退 MySQL；身份存储失败拒绝访问。

网页通过 gRPC-Web Account 注册、登录、me、退出，以 HttpOnly / SameSite=Strict Cookie 保存随机凭据；响应 JSON 不包含 token。Qt 现有 native-login/native-register HTTP 兼容接口由同一后端提供，token 仅在内存保存，原生 gRPC 使用 Authorization metadata。两个入口调用相同账号服务，均按用户校验报告所有权。

密码为 OpenSSL scrypt，既有密码和登录态保持兼容，不写明文密码。生产应使用 HTTPS，设置 INTERVIEW_SECURE_COOKIE=1。历史列表 ListReports 只返回本人报告；GetReport 的 JSON 在网页恢复为问答气泡和总结卡片，原始 JSON 不直接作为用户视图。

可在 MySQL 客户端中查看配置指定库的 users、interview_reports、report_outbox、report_documents，注意不要导出个人回答或凭据。RabbitMQ 的异步文档详见 CLUSTER.md。完整协议见 GRPC_WEB_API.md。
