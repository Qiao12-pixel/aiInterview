# ZooKeeper 注册与 RabbitMQ 报告任务

负载均衡现在内置于每个 C++ 后端，而不是独立网关。Web 仍直接使用 gRPC-Web，Qt 使用原生 gRPC；客户端连接任意后端入口，由后端根据 ZooKeeper 成员快照选择执行节点。

- 新会话：按实例 ID 排序后轮询分配，各入口维护独立游标。
- 回答：解析会话 ID 的实例前缀，固定路由到创建它的节点。
- 本机目标：直接调用业务实现。远端目标：复用原生 gRPC channel，转发一次并保留流式事件、身份、deadline 与取消；不递归转发，不自动重放请求。
- 故障：发现为空时拒绝创建；远端返回 UNAVAILABLE 后冷却 2 秒；临时节点删除后停止向该节点分配新会话。已提交但失去响应的请求不会转发到第二节点。
- 账号、历史、最终报告：由当前入口处理，共享 MySQL/Redis，按原用户身份校验。

模块位置：server/backend/src/infrastructure/cluster/router.*（成员选择/连接复用），src/api/routing.*（RPC 分发），server/discovery/（ZooKeeper 注册发现）。

## 构建与多实例启动

macOS 按需安装/启动 rabbitmq、zookeeper；Linux 编译需要 librabbitmq-dev、libzookeeper-mt-dev。使用根 startup.md 构建。

参考 examples/cluster.example.json，建立两份私有配置，共享 MySQL/Redis、RabbitMQ exchange/queue、ZooKeeper hosts/path，instance_id 分别 node-a/node-b，advertised_address 分别 127.0.0.1:50051 / 50052。

```bash
./build/server/interview_server --config server/config/node-a.json --listen 127.0.0.1:50051 --web-listen 127.0.0.1:8888
./build/server/interview_server --config server/config/node-b.json --listen 127.0.0.1:50052 --web-listen 127.0.0.1:8889
```

浏览器打开 http://127.0.0.1:8888 或 http://127.0.0.1:8889，任一入口都会把新会话分配给两个实例；Qt 使用 --server 127.0.0.1:50051 或 50052 也使用相同分配策略。无需 CORS、浏览器直接访问 ZooKeeper 或独立网关。

启用 ZooKeeper 后，以唯一 instance_id 创建临时节点，约每秒刷新；断开清空发现快照、过期重新注册。实例 ID 仅允许字母、数字、短横线，不复用运行中的 ID。advertised_address 必须是其他后端可连接的原生 gRPC 地址，不能写 Web 端口或 0.0.0.0。注册发现传播和异常会话过期有延迟，不是即时摘除。

节点宕机不恢复其内存面试，会话回答返回明确错误；重启后相同 ID 重新注册并恢复接收新会话，旧未完成会话仍不存在。最终报告可经其他存活入口查询。固定入口本身失效仍需切换客户端地址，公网域名的入口高可用需要部署层或客户端地址列表；应用层路由已解决会话亲和，不再要求部署层做粘滞。

## 验证

```bash
ctest --test-dir build/server --output-on-failure
cmake -S tools/loadtest -B build/loadtest -DCMAKE_BUILD_TYPE=Release
cmake --build build/loadtest --parallel 2
python3 tools/loadtest/cluster_integration.py \
  --backend ./build/server/interview_server \
  --grpc-bench ./build/loadtest/grpc_loadtest
```

集成测试使用本机 ZooKeeper、MySQL root、Redis，随机隔离数据库/Redis前缀/ZooKeeper路径；使用 --mock-model，不调用付费模型。验证 Web/原生gRPC分配、跨入口回答、共享账号、报告读取、强制停止节点、发现摘除、重启重新加入，最后清理测试资源。单元测试覆盖排序轮询、会话归属、故障冷却、空快照、地址变更和防转发环路。

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

`GET /healthz` 返回 cluster_enabled、instance_id、members 和 ready，便于检查当前入口实际看到的发现快照。ok 是 HTTP 进程存活状态，ready 在集群快照为空时为 false；诊断响应不含模型密钥或数据库凭据。节点摘除回归会等待 members 中真正移除失效节点，避免把短暂故障冷却误判为 ZooKeeper 摘除。
