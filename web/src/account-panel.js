export function createAccountPanel(accounts, onReport) {
  const $ = (selector) => document.querySelector(selector);
  let registerMode = false;
  let selectedReport = null;
  function updateSelection() {
    for (const item of $("#reportHistory").querySelectorAll(".history-item")) {
      const selected = item.dataset.sessionId === selectedReport;
      item.classList.toggle("selected", selected);
      if (selected) item.setAttribute("aria-current", "true");
      else item.removeAttribute("aria-current");
    }
  }
  function open() {
    $("#authOverlay").hidden = false;
    $("#authError").textContent = "";
    $("#authEmail").focus();
  }
  $("#accountButton").onclick = async () => {
    if (accounts.user) {
      try {
        await accounts.logout();
      } catch (error) {
        open();
        $("#authError").textContent = error.message;
      }
      return;
    }
    open();
  };
  $("#closeAuth").onclick = () => {
    $("#authOverlay").hidden = true;
  };
  $("#authToggle").onclick = () => {
    registerMode = !registerMode;
    $("#authName").hidden = !registerMode;
    $("#authName").required = registerMode;
    $("#authSubmit").textContent = registerMode ? "创建账号" : "登录";
    $("#authToggle").textContent = registerMode
      ? "已有账号？返回登录"
      : "创建新账号";
    $("#authHint").textContent = registerMode
      ? "使用邮箱和至少 10 位密码创建账号。"
      : "登录后同步保存你的面试记录。";
    $("#authPassword").autocomplete = registerMode
      ? "new-password"
      : "current-password";
  };
  $("#authForm").onsubmit = async (event) => {
    event.preventDefault();
    $("#authError").textContent = "";
    const email = $("#authEmail").value.trim(),
      password = $("#authPassword").value;
    try {
      if (registerMode)
        await accounts.register(email, password, $("#authName").value.trim());
      else await accounts.login(email, password);
    } catch (error) {
      $("#authError").textContent = error.message;
    }
  };
  return {
    open,
    selectReport(id) {
      selectedReport = id;
      updateSelection();
    },
    render(state) {
      $("#accountButton").textContent = state.user
        ? `${state.user.display_name} · 退出`
        : "登录 / 注册";
      $("#authSubmit").disabled = Boolean(state.busy);
      $("#authToggle").disabled = Boolean(state.busy);
      if (state.user) {
        $("#authOverlay").hidden = true;
        $("#accountButton").title = state.user.email;
      }
      if (state.error) $("#authError").textContent = state.error;
    },
    history(reports) {
      const list = $("#reportHistory");
      list.replaceChildren();
      if (!reports.length) {
        const empty = document.createElement("small");
        empty.textContent = accounts.user
          ? "还没有面试记录"
          : "登录后查看历史报告";
        list.append(empty);
        return;
      }
      for (const report of reports) {
        const button = document.createElement("button");
        button.className = "history-item";
        button.dataset.sessionId = report.session_id;
        const title = document.createElement("span");
        title.textContent = report.candidate_name || "技术面试";
        const date = document.createElement("small");
        date.textContent = new Date(report.created_at).toLocaleDateString();
        button.append(title, date);
        button.onclick = () => onReport(report.session_id);
        list.append(button);
      }
      updateSelection();
    },
  };
}
