import { readJson } from "./request-body.js";
import { writeSse } from "./sse-writer.js";
import { authorizeSession } from "./session-auth.js";
import { rejectJson } from "./http-response.js";

export function createReportHandler(client, registry) {
  return async (req, res) => {
    if (req.method !== "POST") return rejectJson(res, 405, "请使用 POST");
    let body;
    try {
      body = await readJson(req, 16 * 1024);
    } catch (error) {
      return rejectJson(res, error.message === "请求过大" ? 413 : 400, error.message);
    }
    const auth = authorizeSession(registry, body);
    if (auth.error) return rejectJson(res, auth.status, auth.error);
    if (auth.session.busy) return rejectJson(res, 409, "该会话正在处理中");

    auth.session.busy = true;
    const stream = writeSse(res);
    let ended = false;
    const finish = (error) => {
      if (ended) return;
      ended = true;
      auth.session.busy = false;
      stream.finish(error);
    };
    client.getReport(
      { session_id: auth.session.id },
      { deadline: new Date(Date.now() + 30000) },
      (error, result) => {
        if (error) {
          finish(error);
          return;
        }
        try {
          stream.send({ type: "report", report: JSON.parse(result.report_json) });
          finish();
        } catch {
          finish({ details: "报告格式无效" });
        }
      },
    );
    stream.onClose(() => {
      if (!ended) {
        call.cancel();
        finish({ details: "连接已断开" });
      }
    });
    return true;
  };
}
