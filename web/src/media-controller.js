import { createVoice } from "./voice.js";
export function createMedia({ onStatus, isBusy, canAnswer }) {
  const $ = (selector) => document.querySelector(selector);
  function update() {
    $("#mic").textContent = voice.listening() ? "结束说话" : "开始说话";
    $("#mic").setAttribute("aria-pressed", String(voice.listening()));
    $("#mic").disabled =
      !voice.supported || (!voice.listening() && (!canAnswer() || isBusy()));
  }
  const voice = createVoice({
    input: $("#textInput"),
    onStatus,
    onChange: update,
  });
  $("#mic").onclick = () => (voice.listening() ? voice.stop() : voice.start());
  $("#stopSpeak").onclick = voice.cancelSpeech;
  $("#mode").onchange = () => {
    voice.stop();
    voice.cancelSpeech();
    onStatus("语音识别由浏览器提供，请确认转写后再提交。");
    update();
  };
  window.addEventListener("pagehide", () => {
    voice.dispose();
  });
  return {
    update,
    stopListening: voice.stop,
    dispose() {
      voice.dispose();
    },
    speak(text) {
      if ($("#mode").value !== "text" && $("#autoSpeak").checked)
        voice.speak(text);
    },
  };
}
