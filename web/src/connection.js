export function createConnection({ onMessage, onState }) {
  let socket;
  function connect() {
    onState("connecting");
    socket = new WebSocket(
      `${location.protocol === "https:" ? "wss:" : "ws:"}//${location.host}/ws`,
    );
    socket.onopen = () => onState("open");
    socket.onclose = () => onState("closed");
    socket.onerror = () => onState("error");
    socket.onmessage = ({ data }) => {
      try {
        onMessage(JSON.parse(data));
      } catch {
        onMessage({ type: "error", message: "服务器消息格式错误" });
      }
    };
  }
  return {
    connect,
    open: () => socket?.readyState === WebSocket.OPEN,
    reset() {
      if (socket) {
        socket.onclose = null;
        socket.onmessage = null;
        socket.close();
      }
      connect();
    },
    disconnect() {
      if (socket) {
        socket.close();
      }
    },
    reconnect() {
      if (!socket || socket.readyState === WebSocket.CLOSED) connect();
    },
    send(type, fields = {}) {
      if (socket?.readyState !== WebSocket.OPEN)
        throw new Error("连接已断开，请重新连接");
      socket.send(
        JSON.stringify({ type, request_id: crypto.randomUUID(), ...fields }),
      );
    },
  };
}
