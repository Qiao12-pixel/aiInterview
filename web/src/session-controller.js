import { createMessages } from "./messages.js";
import { renderReport } from "./report-view.js";
import { createConnection } from "./connection.js";
import { streamAnswer, streamReport } from "./sse.js";
import { createMedia } from "./media-controller.js";
import { createAccountController } from "./account-controller.js";
import { createAccountPanel } from "./account-panel.js";
import { createResumeUpload } from "./resume-upload.js";
import { createSession as createRemoteSession } from "./session-api.js";
export function startInterview() {
  const $ = (selector) => document.querySelector(selector),
    messages = createMessages($("#messages"));
  const resumeUpload = createResumeUpload();
  let accountId = null;
  let historyView = null;
  let liveView = null;
  const replyPlaceholder = $("#textInput").placeholder;
  function saveLiveView() {
    if (!historyView)
      liveView = {
        messages: messages.capture(),
        title: $("#sessionTitle").textContent,
        progress: $("#progress").textContent,
        input: $("#textInput").value,
      };
  }
  function currentInterview() {
    if (busy || !historyView || !liveView) return;
    historyView = null;
    messages.restore(liveView.messages);
    $("#sessionTitle").textContent = liveView.title;
    $("#progress").textContent = liveView.progress;
    $("#textInput").value = liveView.input;
    liveView = null;
    accountPanel?.selectReport(null);
    controls();
  }
  let busy = false,
    session = null,
    completed = false,
    abort = null;
  const media = createMedia({
    onStatus: (text) => ($("#mediaStatus").textContent = text),
    isBusy: () => busy,
    canAnswer: () => Boolean(session && !completed && !historyView),
  });
  let accountPanel;
  const accounts = createAccountController({
    onState: (state) => {
      accountPanel?.render(state);
      if (state.user && accountId !== state.user.id) {
        accountId = state.user.id;
        connection.reset();
        $("#authPassword").value = "";
      } else if (!state.user && accountId) {
        accountId = null;
        connection.disconnect();
        session = null;
        completed = false;
        historyView = null;
        liveView = null;
        accountPanel?.selectReport(null);
        messages.reset();
        media.dispose();
      }
    },
    onReports: (reports) => accountPanel?.history(reports),
  });
  function controls() {
    resumeUpload.setBusy(busy);
    $("#candidateName").disabled = busy;
    $("#questionCount").disabled = busy;
    $("#newSession").disabled = busy || !connection.open();
    $("#textForm button").disabled =
      busy || !session || completed || Boolean(historyView);
    $("#textInput").disabled =
      busy || !session || completed || Boolean(historyView);
    $("#textInput").placeholder = historyView
      ? "历史会话仅供查看，点击“当前面试”返回或新建会话"
      : replyPlaceholder;
    $("#currentInterview").disabled = busy;
    $("#accountButton").disabled = busy;
    $("#currentInterview").classList.toggle("active", !historyView);
    $("#report").disabled = busy || !completed || Boolean(historyView);
    $("#activity").textContent = busy
      ? "服务端正在处理，结果实时显示…"
      : historyView
        ? "历史面试 · 只读回顾，可返回当前面试或新建会话"
        : completed
          ? "面试结束，可查看报告"
          : session
            ? "等待你提交回答"
            : "填写姓名后创建会话";
    media.update();
  }
  function handle(message) {
    if (message.type === "session") {
      session = message;
      completed = false;
      // 流式首题预览已显示，保留气泡，最终 QUESTION 将完成它。
      $("#sessionTitle").textContent =
        `${message.candidate_name} · C++ 后端面试`;
      $("#progress").textContent = `1 / ${message.question_total} 题`;
    } else if (message.type === "event") {
      const payload = message.json_payload
        ? JSON.parse(message.json_payload)
        : {};
      if (message.event_type === "DELTA") {
        messages.preview(payload.target, message.text);
        return;
      }
      messages.finish(message.event_type, message.text);
      if (payload?.question_index)
        $("#progress").textContent =
          `${payload.question_index} / ${session.question_total} 题`;
      if (["QUESTION", "FOLLOW_UP"].includes(message.event_type))
        media.speak(message.text);
      if (message.event_type === "SUMMARY") {
        completed = true;
        accounts
          .loadReports()
          .catch((error) => messages.add("error", error.message));
      }
    } else if (message.type === "report") {
      messages.add("error", "请从历史列表或报告按钮打开面试回顾。");
    } else if (message.type === "error") {
      busy = false;
      messages.fail();
      messages.add("error", message.message);
    } else if (message.type === "done") {
      busy = false;
      if (!completed) $("#textInput").focus();
    }
    controls();
  }
  const connection = createConnection({
    onMessage: handle,
    onState: (state) => {
      const label = $("#connectionText");
      label.textContent = {
        open: "网关已连接",
        connecting: "连接中",
        closed: "连接断开 · 点击重连",
        error: "无法连接网关",
      }[state];
      label.previousElementSibling.style.background =
        state === "open" ? "#42c88a" : "#e58c6a";
      if (state === "closed") {
        abort?.abort();
        session = null;
        busy = false;
        media.dispose();
        messages.fail();
      }
      controls();
    },
  });
  $("#newSession").onclick = async () => {
    if (busy) return;
    const name = $("#candidateName").value.trim();
    if (!name) {
      $("#candidateName").focus();
      return;
    }
    if (!accounts.user) {
      accountPanel.open();
      $("#authError").textContent = "请先登录或创建账号，再开始面试。";
      return;
    }
    busy = true;
    abort = new AbortController();
    controls();
    media.stopListening();
    $("#activity").textContent = resumeUpload.selected
      ? "正在上传简历，服务端解析后生成问题…"
      : "服务端正在生成面试问题…";
    try {
      const pdf = await resumeUpload.payload();
      session = null;
      completed = false;
      historyView = null;
      liveView = null;
      accountPanel?.selectReport(null);
      $("#textInput").value = "";
      $("#progress").textContent = "生成中…";
      messages.reset();
      await createRemoteSession(
        {
          candidate_name: name,
          question_total: Number($("#questionCount").value),
          resume_pdf: pdf,
        },
        handle,
        abort.signal,
      );
    } catch (error) {
      messages.fail();
      messages.add(
        "error",
        error.name === "AbortError"
          ? "创建请求已中止，请确认连接后重试。"
          : error.message,
      );
    } finally {
      busy = false;
      abort = null;
      controls();
    }
  };
  $("#textForm").onsubmit = async (event) => {
    event.preventDefault();
    const input = $("#textInput"),
      text = input.value.trim();
    if (!text || busy || completed || historyView) return;
    if (!accounts.user) {
      $("#authOverlay").hidden = false;
      $("#authError").textContent = "请先登录或创建账号，再开始保存面试记录。";
      return;
    }
    if (!session) return;
    let frame;
    try {
      frame = media.snapshot();
    } catch (error) {
      messages.add("error", error.message);
      return;
    }
    media.stopListening();
    busy = true;
    abort = new AbortController();
    controls();
    messages.add("candidate", text + (frame ? "\n[已附带当前画面]" : ""));
    input.value = "";
    try {
      await streamAnswer(
        {
          session_id: session.id,
          text,
          video_jpeg: frame,
        },
        handle,
        abort.signal,
      );
    } catch (error) {
      messages.fail();
      messages.add(
        "error",
        error.name === "AbortError"
          ? "连接已中止；请确认状态后再操作。"
          : error.message,
      );
      input.value = text;
    } finally {
      busy = false;
      abort = null;
      controls();
    }
  };
  $("#textInput").onkeydown = (event) => {
    if (event.key === "Enter" && !event.shiftKey && !event.isComposing) {
      event.preventDefault();
      $("#textForm").requestSubmit();
    }
  };
  $("#report").onclick = () => {
    if (!busy && session && completed) showReport(session.id);
  };
  async function showReport(sessionId) {
    if (busy) return;
    busy = true;
    abort = new AbortController();
    controls();
    media.stopListening();
    try {
      let report;
      await streamReport(
        { session_id: sessionId },
        (message) => {
          if (message.type === "report") report = message.report;
        },
        abort.signal,
      );
      if (!report) throw new Error("未收到面试记录，请稍后重试");
      saveLiveView();
      const view = renderReport($("#messages"), messages, report);
      historyView = { id: sessionId };
      $("#sessionTitle").textContent = `${view.candidate} · 历史面试`;
      $("#progress").textContent = view.total
        ? `${view.total} / ${view.total} 题 · 已完成`
        : "已完成";
      $("#textInput").value = "";
      accountPanel?.selectReport(sessionId);
    } catch (error) {
      messages.add("error", error.message);
    } finally {
      busy = false;
      abort = null;
      controls();
    }
  }
  $("#currentInterview").onclick = currentInterview;
  accountPanel = createAccountPanel(accounts, showReport);
  $("#connectionText").onclick = connection.reconnect;
  window.addEventListener("pagehide", () => {
    abort?.abort();
    media.dispose();
  });
  connection.connect();
  accounts.restore();
  controls();
}
