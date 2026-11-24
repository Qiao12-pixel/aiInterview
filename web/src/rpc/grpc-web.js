import { encode, decode, concat } from "./protobuf.js";
export function frame(bytes) {
  const out = new Uint8Array(bytes.length + 5);
  new DataView(out.buffer).setUint32(1, bytes.length);
  out.set(bytes, 5);
  return out;
}
export async function call(
  method,
  requestSchema,
  responseSchema,
  payload,
  onMessage,
  signal,
) {
  const response = await fetch(`/aiinterview.v1.InterviewService/${method}`, {
    method: "POST",
    credentials: "same-origin",
    headers: {
      "Content-Type": "application/grpc-web+proto",
      "X-Grpc-Web": "1",
    },
    body: frame(encode(requestSchema, payload)),
    signal,
  });
  if (!response.ok) throw Error(`后端请求失败 HTTP ${response.status}`);
  if (
    !response.headers
      .get("content-type")
      ?.startsWith("application/grpc-web+proto")
  )
    throw Error("后端未返回 gRPC-Web 响应");
  const reader = response.body.getReader();
  let buffer = new Uint8Array(),
    terminal = false,
    result;
  try {
    while (true) {
      const { value, done } = await reader.read();
      if (value) buffer = concat([buffer, value]);
      while (buffer.length >= 5) {
        const n = new DataView(buffer.buffer, buffer.byteOffset).getUint32(1);
        if (n > 8 * 1024 * 1024) throw Error("响应过大");
        if (buffer.length < 5 + n) break;
        if (terminal) throw Error("Unexpected data after trailers");
        const flag = buffer[0],
          bytes = buffer.subarray(5, 5 + n);
        if (flag === 128) {
          const trailers = new TextDecoder().decode(bytes);
          const status = /^grpc-status:\s*(\d+)\s*$/im.exec(trailers);
          if (!status) throw Error("Missing gRPC status");
          if (status[1] !== "0") {
            const message = /^grpc-message:\s*(.*)$/im
              .exec(trailers)?.[1]
              ?.trim();
            throw Error(
              message ? decodeURIComponent(message) : `gRPC ${status[1]}`,
            );
          }
          terminal = true;
        } else if (flag === 0) {
          result = decode(responseSchema, bytes);
          onMessage?.(result);
        } else throw Error("Unsupported gRPC-Web frame");
        buffer = buffer.slice(5 + n);
      }
      if (done) break;
    }
    if (!terminal || buffer.length)
      throw Error("输出流中断，请确认状态后再操作。");
    return result;
  } finally {
    await reader.cancel().catch(() => {});
    reader.releaseLock();
  }
}
