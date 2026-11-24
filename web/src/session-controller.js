import { createMessages } from "./messages.js";
import { createConnection } from "./connection.js";
import { streamAnswer, streamReport } from "./sse.js";
import { createMedia } from "./media-controller.js";
export function startInterview() {
  const $ = (selector) => document.querySelector(selector),
    messages = createMessages($("#messages"));
  let busy = false,
    session = null,
    completed = false,
    abort = null;
  const media = createMedia({
    onStatus: (text) => ($("#mediaStatus").textContent = text),
    isBusy: () => busy,
    canAnswer: () => Boolean(session && !completed),
  });
  function controls() {
    $("#newSession").disabled = busy || !connection.open();
    $("#textForm button").disabled = busy || !session || completed;
    $("#textInput").disabled = busy || !session || completed;
    $("#report").disabled = busy || !completed;
    $("#activity").textContent = busy
      ? "服务端正在处理，结果实时显示…"
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
      messages.reset();
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
      if (message.event_type === "SUMMARY") completed = true;
    } else if (message.type === "report")
      messages.add(
        "assistant",
        JSON.stringify(message.report, null, 2),
        "面试报告",
      );
    else if (message.type === "error") {
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
  function send(type, fields = {}) {
    if (busy) return;
    try {
      connection.send(type, fields);
      busy = true;
      controls();
    } catch (error) {
      messages.add("error", error.message);
    }
  }
  $("#newSession").onclick = () => {
    const name = $("#candidateName").value.trim();
    if (!name) {
      $("#candidateName").focus();
      return;
    }
    media.stopListening();
    send("create_session", {
      candidate_name: name,
      question_total: Number($("#questionCount").value),
    });
  };
  $("#textForm").onsubmit = async (event) => {
    event.preventDefault();
    const input = $("#textInput"),
      text = input.value.trim();
    if (!text || busy || !session || completed) return;
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
          session_token: session.session_token,
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
  $("#report").onclick = async () => {
    if(busy||!session||!completed)return;
    busy=true;abort=new AbortController();controls();
    try {
      await streamReport({session_id:session.id,session_token:session.session_token},message=>{
        if(message.type==="report")messages.add("assistant",JSON.stringify(message.report,null,2),"面试报告");
      },abort.signal);
    } catch(error) { messages.add("error",error.message); }
    finally { busy=false;abort=null;controls(); }
  };
  $("#connectionText").onclick = connection.reconnect;
  window.addEventListener("pagehide", () => {
    abort?.abort();
    media.dispose();
  });
  connection.connect();
  controls();
}
