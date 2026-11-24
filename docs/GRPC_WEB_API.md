# 浏览器直接调用后端

入口 `/aiinterview.v1.InterviewService/<Method>`，POST，Content-Type `application/grpc-web+proto`，请求为单个不压缩 protobuf 帧。后端不提供浏览器原生 gRPC 或 grpc-web-text。

每帧 1 字节标志 + 4 字节网络序长度 + payload。数据标志 0；最后 trailer 标志 0x80，payload 为 `grpc-status` / 百分号编码的 `grpc-message`。无成功 trailer 视为中断；不得自动重放回答。

| 方法 | 请求 | 响应 |
| --- | --- | --- |
| Account | operation/body_json | result_json，注册/登录通过 HttpOnly Cookie 返回凭据 |
| StartInterview | 姓名、题量、PDF 原始字节 | DELTA → SESSION_READY → QUESTION 流 |
| SendText | session_id、text | 评分、追问/下一题、总结流 |
| GetReport | session_id | report_json，由 UI 恢复对话形式 |
| ListReports | offset | 当前用户最近报告列表 |

完整 protobuf 契约见 ../proto/interview.proto。网页使用 web/src/rpc/ 编解码，字段变化需同步；新增未识别字段会跳过。旧 CreateSession unary 继续供 Qt/原生客户端使用。

网页同源部署，Fetch 携带同源 Cookie；拒绝与 Host 不匹配的 Origin，不启用跨域 CORS。不接受客户端伪造的转发 IP。/healthz 返回内置接入状态；静态文件由同进程提供。Qt 原有 `/api/account/native-register`、`native-login` 保留为薄适配，调用相同 Account 服务并返回 access_token；面试使用原生 gRPC。

请求体最多 6 MiB，PDF 最多 5 MiB/20 页，回答最多 32000 字节；账号 JSON 最多 16 KiB。视频字段仍保留旧 wire 编号以兼容既有契约，但 SendText 拒绝视频内容，StreamAudio/StreamVideo 未实现。语音由浏览器转写，再提交文本。

过载返回 HTTP 503 / Retry-After；业务错误使用 gRPC trailer code。生产需通过 TLS 入口部署，并设置 INTERVIEW_SECURE_COOKIE=1 给登录 Cookie 添加 Secure；本机默认明文仅监听回环地址。
