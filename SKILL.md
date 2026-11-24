# 项目开发技能说明

## 适用范围

本项目是 C++17 面试系统，当前包含 Qt6 桌面客户端，并按 `client/`、`server/`、`web/` 分块，Qt 与网页均通过 RPC 接入统一后端。

## 默认工作流

1. 先阅读 `AGENTS.md` 和 `docs/ARCHITECTURE.md`。
2. 确认改动属于界面、协议网关或后端。出题、评分、PDF、模型配置必须在 server/，client/ 仅 UI 和 RPC。
3. 优先新增接口和领域模型，再接入具体实现。
4. 对网络、LLM、PDF、数据库等外部依赖提供可替换适配器。
5. 修改后检查依赖方向、构建配置和敏感信息。

## 质量要求

- C++ 使用 RAII，避免裸资源在异常路径泄漏。
- 服务端 API 返回结构化 JSON，并统一错误码。
- 长耗时操作不得阻塞 UI 线程或 HTTP I/O 线程。
- 日志需脱敏，禁止输出 API key、简历全文和完整回答。
- 新增行为应补充对应的单元或集成测试。

- server/gateway 只适配协议，业务决策位于 server/src/application；应用层依赖端口，具体 LLM/PDF 实现位于 infrastructure。
