# Web 前端

Web 使用原生 HTML、CSS、ES Modules，不需打包。网关提供静态文件和 API：

- `connection.js`：WebSocket 会话创建
- `session-controller.js`：页面状态和操作协调
- `messages.js`：对话 DOM 渲染及 SSE 草稿更新
- `sse.js`：Fetch SSE 分帧解码
- `voice.js` / `camera.js`：语音识别与本地摄像头预览

目前提供语音转写并确认后提交、浏览器语音朗读，以及显式勾选后上传一张 JPEG 画面用于面试回答辅助。**这不是连续实时语音/视频通话**：原始音频、视频不持续上传，未保存媒体。完整双向音视频还需 ASR/TTS/RTC 流接入。

启动命令见项目根目录 `startup.md`。
