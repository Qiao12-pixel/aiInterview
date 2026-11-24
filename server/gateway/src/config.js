import path from "node:path";
import { fileURLToPath } from "node:url";

// 路径锚定模块位置，启动工作目录不影响契约和静态资源定位。
const projectRoot = path.resolve(
  path.dirname(fileURLToPath(import.meta.url)),
  "../../..",
);
export function loadConfig(env = process.env) {
  const port = Number(env.PORT || 8888);
  if (!Number.isInteger(port) || port < 1 || port > 65535)
    throw new Error("PORT 必须为 1–65535 的整数");
  return {
    host: env.HOST || "127.0.0.1",
    port,
    grpcTarget: env.GRPC_TARGET || "127.0.0.1:50051",
    allowedOrigin: env.ALLOWED_ORIGIN || "",
    protoPath: path.resolve(
      env.PROTO_PATH || path.join(projectRoot, "proto/interview.proto"),
    ),
    webRoot: path.resolve(env.WEB_ROOT || path.join(projectRoot, "web")),
  };
}
