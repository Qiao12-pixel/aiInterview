// POST + Fetch 允许携带回答和帧数据；响应为标准 SSE。
export async function streamRequest(path, payload, onMessage, signal) {
  const response = await fetch(path, {
    method: "POST",
    headers: {
      "Content-Type": "application/json",
      Accept: "text/event-stream",
    },
    body: JSON.stringify(payload),
    signal,
  });
  if (!response.ok) {
    const error = await response.json().catch(() => ({}));
    throw new Error(error.message || `HTTP ${response.status}`);
  }
  const reader = response.body.getReader(),
    decoder = new TextDecoder();
  let buffer = "",
    terminal = false;
  try {
    while (true) {
      const { value, done } = await reader.read();
      buffer += decoder.decode(value, { stream: !done });
      let newline;
      while ((newline = buffer.indexOf("\n")) !== -1) {
        const line = buffer.slice(0, newline).replace(/\r$/, "");
        buffer = buffer.slice(newline + 1);
        if (!line.startsWith("data:")) continue;
        const message = JSON.parse(line.slice(5).trim());
        if (message.type === "error") throw new Error(message.message);
        if (message.type === "done") terminal = true;
        onMessage(message);
      }
      if (done) break;
    }
    if (!terminal)
      throw new Error("输出流中断；请确认会话状态后再操作，避免重复提交。");
  } finally {
    reader.releaseLock();
  }
}

export const streamAnswer = (payload, onMessage, signal) =>
  streamRequest("/api/answer", payload, onMessage, signal);
export const streamReport = (payload, onMessage, signal) =>
  streamRequest("/api/report", payload, onMessage, signal);
