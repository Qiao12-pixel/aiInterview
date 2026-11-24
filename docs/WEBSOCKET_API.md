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
