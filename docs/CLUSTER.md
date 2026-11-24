# ZooKeeper 注册与 RabbitMQ 报告任务

网关已移除，原来的网关轮询/会话路由不再运行。后端仍可向 ZooKeeper 注册原生 gRPC 地址，RabbitMQ/outbox 与报告任务继续保留。浏览器不访问 ZooKeeper；当前每个网页入口直接连接一个后端。统一多实例入口需部署层提供会话亲和的 gRPC-Web HTTP 负载均衡，不能把进行中会话随机切换实例。

## 构建与多实例启动

macOS 按需安装/启动 rabbitmq、zookeeper；Linux 编译需要 librabbitmq-dev、libzookeeper-mt-dev。使用根 startup.md 构建。

参考 examples/cluster.example.json，建立两份私有配置，共享 MySQL/Redis、RabbitMQ exchange/queue、ZooKeeper hosts/path，instance_id 分别 node-a/node-b，advertised_address 分别 127.0.0.1:50051 / 50052。

```bash
./build/server/interview_server --config server/config/node-a.json --listen 127.0.0.1:50051 --web-listen 127.0.0.1:8888
./build/server/interview_server --config server/config/node-b.json --listen 127.0.0.1:50052 --web-listen 127.0.0.1:8889
```

Web 入口分别 8888 / 8889，Qt --server 显式选实例。启用 ZooKeeper 后，以唯一 instance_id 创建临时节点，约每秒刷新；断开清空发现快照、过期重新注册。当前保留基础设施，不承诺浏览器自动发现/均衡。实例 ID 仅字母数字短横线，不复用正在运行的 ID。

未完成会话依旧保存在创建它的进程内存，进程失效无法恢复；共享数据库只让最终报告和账号可跨实例读取。不得自动重试已经提交的回答。

## 异步报告任务

启用 rabbitmq.enabled 后，最终报告和 outbox 记录在同一个 MySQL 事务提交。后台启动两个 worker：

```bash
./build/server/backend/report_event_worker --config server/config/node-a.json
./build/server/backend/report_task_worker --config server/config/node-a.json
```

也支持 --once：处理当前待发送/待消费消息后退出，用于验证。多个 outbox worker 用 MySQL FOR UPDATE SKIP LOCKED 竞争任务，需要 MySQL 8+。消费者写入 report_documents 成功才 ACK；崩溃后可能再次投递，唯一键实现幂等。

RabbitMQ 停止不会阻止面试完成，因为消息已进入 MySQL outbox。发布 worker 保留未发送记录，恢复后重试。发送确认与 MySQL 标记之间存在至少一次投递窗口，消费者必须按 event_id 去重，不能承诺 exactly-once。

当前消费失败会 requeue 并在 worker 层退避 1 秒，不具备失败次数上限和死信队列；毒消息可能阻塞 --once 或反复重试，正式上线前需补失败告警/死信和限次策略。report_documents 暂用于异步文本产物，尚未提供前端下载入口。outbox 和文档表需后续设置保留清理策略。

## 部署限制

ZooKeeper 当前开发 ACL、RabbitMQ 本地 guest 不能直接用于公网。正式部署需要独立用户、认证/TLS 和私网隔离。原始网关多实例压测结果保留于 benchmarks/2026-11-24/REPORT.md，仅代表旧架构。
