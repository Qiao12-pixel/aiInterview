import { call } from "./grpc-web.js";
const types = [
  "TYPE_UNSPECIFIED",
  "QUESTION",
  "TRANSCRIPT",
  "FEEDBACK",
  "FOLLOW_UP",
  "SUMMARY",
  "ERROR",
  "DELTA",
  "SESSION_READY",
];
function event(value, onMessage) {
  const payload = value.json_payload ? JSON.parse(value.json_payload) : {};
  if (value.type === 8) onMessage({ type: "session", ...payload });
  else onMessage({ ...value, type: "event", event_type: types[value.type] });
}
export const createSession = (payload, onMessage, signal) =>
  call(
    "StartInterview",
    "CreateSessionRequest",
    "InterviewEvent",
    payload,
    (v) => event(v, onMessage),
    signal,
  ).then(() => onMessage({ type: "done" }));
export const streamAnswer = (payload, onMessage, signal) =>
  call(
    "SendText",
    "TextMessage",
    "InterviewEvent",
    payload,
    (v) => event(v, onMessage),
    signal,
  ).then(() => onMessage({ type: "done" }));
export const streamReport = (payload, onMessage, signal) =>
  call(
    "GetReport",
    "ReportRequest",
    "ReportResponse",
    payload,
    (v) => onMessage({ type: "report", report: JSON.parse(v.report_json) }),
    signal,
  ).then(() => onMessage({ type: "done" }));
export const accountRequest = async (operation, body = {}) =>
  JSON.parse(
    (
      await call("Account", "AccountRequest", "AccountResponse", {
        operation,
        body_json: JSON.stringify(body),
      })
    ).result_json,
  );
export const listReports = async () =>
  JSON.parse(
    (
      await call("ListReports", "ListReportsRequest", "ListReportsResponse", {
        offset: 0,
      })
    ).reports_json,
  );
