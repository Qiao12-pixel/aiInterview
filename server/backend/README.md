# C++ 面试后端

唯一的业务实现，不依赖 Qt 或客户端源码。入口 `src/api/main.cpp`，会话状态和业务编排位于 `src/application/engine.cpp`，模型适配器与 PDF 解析位于 `src/infrastructure/`。接口和生成目标来自 `../../proto/`。头文件与实现同处 src/，没有独立 include/。

已实现：创建会话、PDF 简历驱动出题、文本回答、结构化评分、每题最多一次追问和最终报告。评分与总结支持 JSON SSE 增量事件；视频辅助请求最多携带一张 JPEG 帧，只有客户端明确勾选时才上传。不会保存原始媒体。

浏览器语音转写通过 Web Speech API，由用户确认文本后提交；问题朗读使用浏览器 Speech Synthesis，摄像头仅在本地预览。该实现不是连续实时音视频通话，完整 RTC/ASR/TTS 流尚未接入。模型参数仅从服务端 JSON 配置读取。`--mock` 可离线验证 RPC 和 PDF 解析。

依赖：C++17、CMake、gRPC/Protobuf、libcurl、nlohmann_json、pkg-config、poppler-cpp。PDF 解析已由原客户端 PoDoFo 实现替换为 Poppler C++，支持 Linux 和 macOS。

构建、测试和网关启动见项目根目录 `startup.md`。

## 服务端目录

- `src/api`：gRPC 接入与依赖组装
- `src/application`：业务用例与状态流转
- `src/domain`：会话与事件数据
- `src/ports`：业务依赖的模型接口
- `src/infrastructure`：模型 HTTP、PDF 等具体适配器
- `../gateway`：并列的独立 C++ 网关进程
- `../config`：服务端私有配置，仅后端读取

应用层不链接 HTTP、PDF 或 gRPC 库。网关通过契约访问 API，不导入 C++ 实现；Qt 和 Web 不依赖任何服务端源码。

账号与报告持久化：`ports/report_store.h` 隔离应用与 MySQL/Redis，`infrastructure/storage/` 提供实现；登录与密码模块在 `src/accounts/`，gRPC 端独立验证 Redis 登录态和用户归属。见 `../../docs/ACCOUNTS_AND_STORAGE.md`。

数据库初始化现已内置 C++ 启动流程，见 `src/infrastructure/database/bootstrap.*`；启用 storage 时自动创建缺少的库、表和必要字段/索引，无需手动执行建表工具。

可独立构建：`cmake -S server/backend -B build/backend`；或通过 `cmake -S server -B build/server` 同时构建后端和网关。
