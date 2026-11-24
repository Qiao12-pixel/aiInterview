import { accountRequest, listReports } from "./rpc/interview.js";
export function createAccountController({ onState, onReports }) {
  let busy = false;
  const request = async (action, body = {}) => {
    if (busy) throw new Error("账号请求处理中");
    busy = true;
    onState({ user: account.user, busy: true });
    try {
      const result = await accountRequest(action, body);
      if (result.user) account.user = result.user;
      if (action === "logout") account.user = null;
      onState({ user: account.user });
      return result;
    } finally {
      busy = false;
      onState({ user: account.user, busy: false });
    }
  };
  const account = {
    user: null,
    async restore() {
      try {
        await request("me");
        await this.loadReports();
      } catch {
        this.user = null;
        onState({ user: null });
        onReports([]);
      }
    },
    async register(email, password, display_name) {
      const result = await request("register", {
        email,
        password,
        display_name,
      });
      await this.loadReports();
      return result;
    },
    async login(email, password) {
      const result = await request("login", { email, password });
      await this.loadReports();
      return result;
    },
    async logout() {
      await request("logout");
      onReports([]);
    },
    async loadReports() {
      if (!this.user) return onReports([]);
      const result = await listReports();
      onReports(Array.isArray(result) ? result : result.reports || []);
    },
  };
  return account;
}
