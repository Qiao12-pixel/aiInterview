# Web 前端

浏览器使用原生 HTML/CSS/ES Modules，无需打包。网页静态资源及账号、简历、WebSocket/SSE 接口都由 C++ 网关提供，不需要运行 npm 或独立静态服务器。

```bash
./build/server/gateway/interview_gateway
```

打开 http://127.0.0.1:8888，后端 gRPC 服务和 MySQL/Redis 需同时运行，完整构建/启动见根目录 startup.md。

模块：connection（WebSocket）、session-controller（状态与交互）、messages（气泡）、sse（SSE 解码）、resume-upload/session-api（PDF）、account-controller/account-panel（账号与历史）、voice/camera/media-controller（浏览器媒体辅助）。

语音由浏览器转写后确认提交，题目可朗读；摄像头本地预览，勾选时随回答上传单张 JPEG。这不是连续实时 RTC 音视频通话。简历仅支持 5 MiB、20 页以内可提取文字的 PDF，扫描件不支持 OCR。

历史报告使用 report-model.js/report-view.js 转为原问答顺序的对话回顾，显示每轮评分、综合评分与建议，隐藏内部 ID 和原始 JSON。历史只读；切换时保存当前会话气泡、滚动位置和输入草稿，点击“当前面试”恢复。旧报告若未保存逐轮问答会明确提示，不虚构记录。测试：`node --test web/tests/report-view.test.js`（Node 仅为可选测试工具，运行服务不依赖它）。
