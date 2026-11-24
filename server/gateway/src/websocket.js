import { WebSocket } from "ws";
import { createSession } from "./rpc/create-session.js";
import { answer } from "./rpc/answer.js";
import { report } from "./rpc/report.js";
const handlers = { create_session: createSession, answer, get_report: report };
export function handleConnection(ws, client, registry) {
  const state = { sessionId: null, busy: false, active: null };
  const send = (data) => {
    if (ws.readyState === WebSocket.OPEN) ws.send(JSON.stringify(data));
  };
  send({ type: "connected", message: "已连接面试服务网关" });
  ws.on("close", () => state.active?.cancel());
  ws.on("message", (raw, binary) => {
    let data;
    try {
      if (binary) throw Error();
      data = JSON.parse(raw.toString());
    } catch {
      send({ type: "error", message: "只接受 JSON 文本消息" });
      return;
    }
    if (
      !data ||
      typeof data !== "object" ||
      Array.isArray(data) ||
      typeof data.type !== "string"
    ) {
      send({ type: "error", message: "消息必须是包含 type 的 JSON 对象" });
      return;
    }
    const id = data.request_id;
    if (state.busy) {
      send({
        type: "error",
        request_id: id,
        message: "上一请求正在处理中，请等待",
      });
      return;
    }
    const handler = handlers[data.type];
    if (!handler) {
      send({ type: "error", request_id: id, message: "不支持的消息类型" });
      return;
    }
    state.busy = true;
    let finished = false;
    const context = {
      state,
      registry,
      options: { deadline: new Date(Date.now() + 150000) },
      send: (event) => send({ ...event, request_id: id }),
      finish: (error) => {
        if (finished) return;
        finished = true;
        state.busy = false;
        state.active = null;
        send(
          error
            ? {
                type: "error",
                request_id: id,
                message: error.details || "请求失败",
              }
            : { type: "done", request_id: id },
        );
      },
    };
    try {
      const call = handler(client, context, data);
      if (!finished) state.active = call;
    } catch {
      context.finish({ details: "请求处理失败" });
    }
  });
}
