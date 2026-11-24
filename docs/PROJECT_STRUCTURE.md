# 项目目录说明

先看“谁运行”，再看进程内部“谁负责什么”。顶层只表达用户端、服务端、通信协议和文档。

```text
aiInterview/
├── client/                         Qt 桌面客户端
│   ├── CMakeLists.txt              Qt 独立构建入口
│   ├── src/
│   │   ├── main_qt.cpp             桌面程序启动
│   │   ├── ui/                     窗口、布局、会话交互
│   │   ├── account/                登录界面与账号 API 调用
│   │   └── transport/              gRPC 客户端
│   └── tests/                      Qt/客户端测试
├── web/                            浏览器前端
│   ├── index.html                  页面结构
│   ├── src/                        对话、账号、历史回放、上传、媒体交互
│   └── tests/                      展示逻辑测试
├── server/                         全部服务端进程
│   ├── CMakeLists.txt              服务端聚合构建入口
│   ├── backend/                    业务后端（interview_server）
│   │   ├── CMakeLists.txt          后端独立构建入口
│   │   ├── src/
│   │   │   ├── api/                gRPC 接口与启动组装
│   │   │   ├── accounts/           注册、登录、密码与用户仓储
│   │   │   ├── application/        面试用例、状态推进、报告
│   │   │   ├── domain/             面试会话、事件数据
│   │   │   ├── ports/              模型、报告、身份接口
│   │   │   ├── infrastructure/     模型 HTTP、PDF、数据库与 Redis 实现
│   │   │   └── common/             本进程共用的 JSON/编码工具
│   │   └── tests/                  业务、解析、存储测试
│   ├── gateway/                    协议网关（interview_gateway）
│   │   ├── CMakeLists.txt          网关独立构建入口
│   │   ├── src/
│   │   │   ├── main.cpp            网关启动
│   │   │   ├── config.h/.cpp       端口、上游和静态目录配置
│   │   │   ├── routes/             Web API 到 RPC 的映射
│   │   │   ├── transport/          HTTP、WebSocket、SSE、gRPC
│   │   │   └── common/             协议字段、上传字节校验
│   │   └── tests/                  网关及全链路测试
│   └── config/                     业务后端私有配置，不进入网关
├── proto/                          Qt、网关和业务后端共用的协议契约
├── docs/                           架构、API、账号、配置示例
├── startup.md                      本地构建与启动步骤
└── CMakeLists.txt                  Qt/后端/网关选择性构建入口
```

头文件与实现放在模块旁，例如 `backend/src/application/engine.h/.cpp`；不设置独立 include 树，不创建 shared 业务目录。

## 新功能放在哪里

| 想修改的内容 | 位置 |
| --- | --- |
| 网页 UI、历史对话展示 | `web/src/` |
| Qt 窗口和操作 | `client/src/ui/` |
| 注册登录、密码规则 | `server/backend/src/accounts/` |
| 出题流程、评分编排、报告提交 | `server/backend/src/application/` |
| 模型 API、PDF、数据库连接与自动建表 | `server/backend/src/infrastructure/` |
| 浏览器请求如何转成 RPC、SSE 心跳 | `server/gateway/src/` |
| 跨端增加接口或字段 | `proto/interview.proto` |

## 依赖方向

网页通过网关调用业务后端，Qt 直接调用面试 gRPC；账号 HTTP 由网关转给后端。网关和 Qt 只引用协议，不引用 backend 源码。application 依赖 domain/ports，具体模型与存储由 infrastructure 适配器提供。账号数据库只由 backend 访问。

## 开发环境文件

`build/`、`cmake-build-*` 是生成的构建目录，不属于源码分层；IDE 文件、日志、测试样例和历史复盘也不是运行模块。旧 `testModule/`、`resource/` 为早期试验/样例资料，未自动删除或搬动个人资料。
