import { createVoice } from "./voice.js";
import { createCamera } from "./camera.js";
export function createMedia({ onStatus, isBusy, canAnswer }) {
  const $ = (selector) => document.querySelector(selector);
  function update() {
    $("#mic").textContent = voice.listening() ? "结束说话" : "开始说话";
    $("#mic").setAttribute("aria-pressed", String(voice.listening()));
    $("#mic").disabled =
      !voice.supported || (!voice.listening() && (!canAnswer() || isBusy()));
    $("#camera").disabled = camera.pending();
    $("#camera").textContent = camera.active() ? "关闭摄像头" : "开启摄像头";
    $("#camera").setAttribute("aria-pressed", String(camera.active()));
    $("#attachFrame").disabled = $("#mode").value !== "video";
  }
  const voice = createVoice({
    input: $("#textInput"),
    onStatus,
    onChange: update,
  });
  const camera = createCamera({
    video: $("#cameraPreview"),
    placeholder: $("#videoPlaceholder"),
    onStatus,
    onChange: update,
  });
  $("#mic").onclick = () => (voice.listening() ? voice.stop() : voice.start());
  $("#camera").onclick = () => camera.toggle();
  $("#stopSpeak").onclick = voice.cancelSpeech;
  $("#mode").onchange = () => {
    voice.stop();
    voice.cancelSpeech();
    if ($("#mode").value !== "video") {
      $("#attachFrame").checked = false;
      camera.stop();
    }
    onStatus(
      $("#mode").value === "video"
        ? "可开启预览并选择附带画面，语音转写后确认提交。"
        : "语音识别由浏览器提供，请确认转写后再提交。",
    );
    update();
  };
  window.addEventListener("pagehide", () => {
    voice.dispose();
    camera.stop();
  });
  return {
    update,
    stopListening: voice.stop,
    dispose() {
      voice.dispose();
      camera.stop();
    },
    snapshot() {
      if (!$("#attachFrame").checked) return "";
      if ($("#mode").value !== "video")
        throw new Error("请先选择视频辅助对话模式。");
      return camera.snapshot();
    },
    speak(text) {
      if ($("#mode").value !== "text" && $("#autoSpeak").checked)
        voice.speak(text);
    },
  };
}
