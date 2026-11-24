# 本机隔离压测

仅用测试模型，无付费调用、真实简历或用户数据。使用真实本机 MySQL/Redis，脚本随机创建并清理测试库和 Redis 前缀，不停止其他服务。需要 mysql root 本机访问权限、redis-cli、Python3。

```bash
cmake -S server -B build/bench-server -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=OFF
cmake --build build/bench-server --parallel 3
cmake -S tools/loadtest -B build/loadtest -DCMAKE_BUILD_TYPE=Release
cmake --build build/loadtest --parallel 2
python3 -u tools/loadtest/run_upgrade.py \
  --backend ./build/bench-server/interview_server \
  --grpc-bench ./build/loadtest/grpc_loadtest \
  --output docs/benchmarks/local/upgrade-results.json
```

每档原生 gRPC 与 gRPC-Web 顺序测试，并发 1/8/32/64/128/160，请求数 max(100,并发×2)。每档独立进程、Redis 前缀，使用隔离账号，创建一题→回答/保存→查报告为一个完整流程。MySQL/Redis 连接池分别 8/16。报告成功后额外验证历史列表和退出失效。token 不写入结果。

短时同机测量不能替代长期稳定性或真实 LLM 容量；出现过载按 HTTP 503 统计，不自动重试。默认32工作线程、最多256处理/排队请求，空闲HTTP连接异步等待且有读超时。

旧 run_local.py / read_connections.py 随网关移除，不再适用；旧报告原始结果保留，升级对比以同机/同模型的完整流程为口径。当前脚本不复测旧网关双实例轮询，也不包含 RabbitMQ 任务吞吐；MQ 旧结果不可直接当作本轮复测。
