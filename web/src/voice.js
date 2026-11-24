export function createVoice({ input, onStatus, onChange }) {
  const Recognition =
    window.SpeechRecognition || window.webkitSpeechRecognition;
  let recognition = null,
    listening = false,
    base = "",
    final = "";
  if (Recognition) {
    recognition = new Recognition();
    recognition.lang = "zh-CN";
    recognition.continuous = true;
    recognition.interimResults = true;
    recognition.onstart = () => {
      listening = true;
      onStatus("正在聆听…结束后请确认转写内容再提交。");
      onChange();
    };
    recognition.onresult = (event) => {
      let interim = "";
      for (let i = event.resultIndex; i < event.results.length; i++) {
        if (event.results[i].isFinal) final += event.results[i][0].transcript;
        else interim += event.results[i][0].transcript;
      }
      input.value = base + final + interim;
    };
    recognition.onerror = (event) =>
      onStatus(`语音识别不可用（${event.error}），可继续输入文字。`);
    recognition.onend = () => {
      listening = false;
      onChange();
    };
  }
  function stop() {
    recognition?.stop();
  }
  function cancelSpeech() {
    window.speechSynthesis?.cancel();
  }
  return {
    supported: Boolean(Recognition),
    listening: () => listening,
    stop,
    cancelSpeech,
    start() {
      if (!recognition) {
        onStatus("当前浏览器不支持语音识别，请使用支持 Web Speech 的浏览器。");
        return;
      }
      cancelSpeech();
      base = input.value;
      final = "";
      try {
        recognition.start();
      } catch {
        onStatus("无法启动语音识别，请检查麦克风权限。");
      }
    },
    speak(text) {
      if (!window.speechSynthesis) {
        onStatus("当前浏览器不支持朗读。");
        return;
      }
      cancelSpeech();
      const utterance = new SpeechSynthesisUtterance(text);
      utterance.lang = "zh-CN";
      window.speechSynthesis.speak(utterance);
    },
    dispose() {
      recognition?.abort();
      cancelSpeech();
    },
  };
}
