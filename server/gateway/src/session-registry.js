import { randomBytes } from "node:crypto";
export class SessionRegistry {
  constructor() {
    this.sessions = new Map();
  }
  issue(id) {
    for (const [token, entry] of this.sessions)
      if (entry.expires < Date.now()) this.sessions.delete(token);
    if (this.sessions.size >= 1000) throw new Error("网关会话容量已满");
    const token = randomBytes(32).toString("hex");
    this.sessions.set(token, {
      id,
      expires: Date.now() + 2 * 60 * 60 * 1000,
      busy: false,
    });
    return token;
  }
  find(token, id) {
    const entry = this.sessions.get(token);
    return entry?.id === id && entry.expires > Date.now() ? entry : null;
  }
}
