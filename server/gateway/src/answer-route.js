import { readJson } from "./request-body.js";
import { writeSse } from "./sse-writer.js";
import { authorizeSession } from "./session-auth.js";
import { rejectJson } from "./http-response.js";

export function createAnswerHandler(client, registry) {
  return async (req, res) => {
    if (req.method !== "POST") return rejectJson(res, 405, "请使用 POST");
    let body;
    try {
      body = await readJson(req);
    } catch (error) {
      return rejectJson(res, error.message === "请求过大" ? 413 : 400, error.message);
    }
    if (
      !body ||
      typeof body.text !== "string" ||
      !body.text.trim() ||
      Buffer.byteLength(body.text) > 32000
    )
      return rejectJson(res, 400, "回答须为 1–32000 字节");
    const auth=authorizeSession(registry,body);
    if(auth.error)return rejectJson(res,auth.status,auth.error);
    const entry=auth.session;
    if (entry.busy) return rejectJson(res, 409, "该会话正在处理中");
    let jpeg = Buffer.alloc(0);
    if (body.video_jpeg) {
      if (
        typeof body.video_jpeg !== "string" ||
        !/^[A-Za-z0-9+/]*={0,2}$/.test(body.video_jpeg)
      )
        return rejectJson(res, 400, "无效画面");
      jpeg = Buffer.from(body.video_jpeg, "base64");
      if (jpeg.length > 512 * 1024 || jpeg[0] !== 0xff || jpeg[1] !== 0xd8)
        return rejectJson(res, 400, "画面必须为不超过 512 KiB 的 JPEG");
    }
    entry.busy = true;
    const stream = writeSse(res);
    stream.send({ type: "status", message: "服务端正在分析回答" });
    const call = client.sendText(
      { session_id: entry.id, text: body.text.trim(), video_jpeg: jpeg },
      { deadline: new Date(Date.now() + 150000) },
    );
    const heartbeat = setInterval(stream.heartbeat, 15000);
    function finish(error) {
      if (!stream.finish(error)) return;
      entry.busy = false;
      clearInterval(heartbeat);
    }
    call.on("data", (event) => {
      if (!stream.send({ ...event, type: "event", event_type: event.type })) call.pause();
    });
    stream.onDrain(() => call.resume());
    call.on("error", finish);
    call.on("end", () => finish());
    stream.onClose(() => {
      if (!stream.closed()) {
        call.cancel();
        finish({ details: "连接已断开" });
      }
    });
    return true;
  };
}
