export function writeSse(response) {
  response.writeHead(200, {
    "Content-Type": "text/event-stream; charset=utf-8",
    "Cache-Control": "no-cache, no-transform",
    "X-Accel-Buffering": "no",
  });
  response.flushHeaders();
  let ended = false;
  return {
    send(data) { return response.write(`data: ${JSON.stringify(data)}\n\n`); },
    heartbeat() { if (!response.destroyed) response.write(": heartbeat\n\n"); },
    onDrain(callback) { response.on("drain", callback); },
    onClose(callback) { response.on("close", callback); },
    finish(error) {
      if (ended) return false;
      ended = true;
      if (!response.destroyed) {
        this.send(error ? { type: "error", message: error.details || "后端请求失败" } : { type: "done" });
        response.end();
      }
      return true;
    },
    closed() { return ended; },
  };
}
