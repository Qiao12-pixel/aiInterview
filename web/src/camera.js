export function createCamera({ video, placeholder, onStatus, onChange }) {
  let stream = null,
    pending = false;
  function stop() {
    stream?.getTracks().forEach((track) => track.stop());
    stream = null;
    video.srcObject = null;
    video.hidden = true;
    placeholder.hidden = false;
    onChange();
  }
  return {
    active: () => Boolean(stream),
    pending: () => pending,
    stop,
    async toggle() {
      if (stream) {
        stop();
        onStatus("摄像头已关闭。");
        return;
      }
      if (!navigator.mediaDevices?.getUserMedia) {
        onStatus("摄像头需要 HTTPS 或本机地址访问。");
        return;
      }
      pending = true;
      onChange();
      try {
        stream = await navigator.mediaDevices.getUserMedia({
          video: { width: { ideal: 640 }, height: { ideal: 480 } },
          audio: false,
        });
        video.srcObject = stream;
        video.hidden = false;
        placeholder.hidden = true;
        stream.getVideoTracks()[0].onended = stop;
        onStatus(
          "预览已开启；勾选附带画面后，提交回答时上传一帧，不保存原始媒体。",
        );
      } catch {
        onStatus("无法打开摄像头，请检查设备和权限。");
      } finally {
        pending = false;
        onChange();
      }
    },
    snapshot() {
      if (!stream || !video.videoWidth)
        throw new Error("请先开启摄像头，或取消附带画面。");
      const canvas = document.createElement("canvas"),
        scale = Math.min(1, 640 / video.videoWidth);
      canvas.width = Math.round(video.videoWidth * scale);
      canvas.height = Math.round(video.videoHeight * scale);
      canvas
        .getContext("2d")
        .drawImage(video, 0, 0, canvas.width, canvas.height);
      return canvas.toDataURL("image/jpeg", 0.65).split(",")[1];
    },
  };
}
