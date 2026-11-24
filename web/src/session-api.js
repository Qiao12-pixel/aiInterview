import { streamRequest } from "./sse.js";
export async function createSession(payload, onMessage, signal) {
  return streamRequest("/api/sessions", payload, onMessage, signal);
}
