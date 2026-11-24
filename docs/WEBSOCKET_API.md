# WebSocket 网关协议 v1

地址：同源 `/ws`，JSON 文本帧，单帧最多 64 KiB。一个连接绑定一个会话；同一连接最多一个处理中请求。WebSocket 消息 `type` 与 gRPC 事件 `event_type` 分开，避免字段冲突。

创建会话：

```json
{"type":"create_session","request_id":"唯一请求ID","candidate_name":"候选人","question_total":3}
```

响应依次为 `session`（id、candidate_name、question_index、question_total）、`event`（QUESTION）、`done`。

提交回答：

```json
{"type":"answer","request_id":"唯一请求ID","text":"回答内容"}
```

响应依次为 `event`（FEEDBACK）、`event`（QUESTION/FOLLOW_UP/SUMMARY）、`done`。回答使用当前连接的 session_id，忽略客户端提供的其他会话 ID。

查询报告：

```json
{"type":"get_report","request_id":"唯一请求ID"}
```

返回 `report`（report 对象）和 `done`。面试未完成时返回错误。

事件示例：

```json
{"type":"event","event_type":"QUESTION","request_id":"唯一请求ID","session_id":"会话ID","text":"问题正文","json_payload":"{\"question_index\":2,\"question_total\":3}"}
```

错误统一为：

```json
{"type":"error","request_id":"唯一请求ID","message":"可读错误原因"}
```

错误是当前请求的终止消息，不保证随后还有 `done`。会话重启、断线恢复和请求去重未实现；不可自动重放已提交回答。媒体帧暂不接受，未来新增带序号/时间戳的二进制消息协议。

## Web 创建会话及简历上传

网页新建会话使用 `POST /api/sessions`（同源登录 Cookie）。JSON 字段为 candidate_name、question_total 和可选 resume_pdf（PDF 原始字节的 Base64，原始文件最大 5 MiB）。请求体最大 7 MiB，成功返回 session、first_question、session_token。网关转为同一 CreateSession gRPC，避免现有 64 KiB WebSocket 消息限额限制简历。原 WebSocket create_session 仍可用于无简历调用。

### 首题 SSE

`POST /api/sessions` 指定 `Accept: text/event-stream` 时，响应依次包含 status、若干 event/DELTA（json_payload.target=QUESTION）、session、event/QUESTION、done。生成失败返回 error 而不返回成功 done。草稿不代表会话已经可回答，必须等待正式 session 和 QUESTION。

链路为模型 SSE → C++ StartInterview gRPC 服务端流 → 网关 SSE → 浏览器；不是前端定时拆字。JSON 兼容模式在没有 SSE Accept 头时保留。

网关已迁移为 C++，API、WebSocket 消息及 SSE 事件结构保持兼容；源码与参数见 server/gateway/README.md。
