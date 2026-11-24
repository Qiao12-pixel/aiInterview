import http from "node:http";
import { WebSocketServer } from "ws";
import { loadConfig } from "./config.js";
import { createGrpcClient } from "./grpc-client.js";
import { createHttpHandler } from "./http.js";
import { handleConnection } from "./websocket.js";
import { SessionRegistry } from "./session-registry.js";
import { createAnswerHandler } from "./answer-route.js";
import { createReportHandler } from "./report-route.js";

const config = loadConfig();
const client = createGrpcClient(config);
const registry = new SessionRegistry();
const apiHandlers = {
  "/api/answer": createAnswerHandler(client, registry),
  "/api/report": createReportHandler(client, registry),
};
const server = http.createServer(createHttpHandler(config.webRoot, apiHandlers));
const wss = new WebSocketServer({ noServer: true, maxPayload: 64 * 1024 });
server.on("upgrade", (req, socket, head) => {
  const expectedOrigin = config.allowedOrigin || `http://${req.headers.host}`;
  if (req.url !== "/ws" || req.headers.origin !== expectedOrigin) {
    socket.destroy();
    return;
  }
  wss.handleUpgrade(req, socket, head, (ws) => wss.emit("connection", ws));
});
wss.on("connection", (ws) => handleConnection(ws, client, registry));
server.listen(config.port, config.host, () => {
  const address = server.address();
  const urlHost = address.address.includes(":")
    ? `[${address.address}]`
    : address.address;
  console.log(`Web/WS gateway: http://${urlHost}:${address.port}`);
  console.log(`gRPC upstream: ${config.grpcTarget}`);
  if (config.host === "127.0.0.1")
    console.log(
      "请使用上方 IPv4 地址；localhost 可能通过 IPv6 访问到其他服务。",
    );
});

function shutdown() {
  wss.clients.forEach((ws) => ws.terminate());
  server.close(() => {
    client.close();
    process.exit(0);
  });
}

process.on("SIGTERM", shutdown);
process.on("SIGINT", shutdown);
