import test from "node:test";
import assert from "node:assert/strict";
import WebSocket from "ws";

const address = process.env.TEST_GATEWAY || "http://127.0.0.1:8081";
function open(origin = address) {
  return new WebSocket(address.replace("http", "ws") + "/ws", { origin });
}

test("WebSocket creates sessions and rejects malformed input", async () => {
  const ws = open();
  const queue = [];
  let waiter;
  ws.on("message", (raw) => {
    queue.push(JSON.parse(raw));
    waiter?.();
  });
  async function next() {
    while (!queue.length)
      await new Promise((resolve, reject) => {
        const timeout = setTimeout(() => reject(Error("timeout")), 5000);
        waiter = () => {
          clearTimeout(timeout);
          waiter = null;
          resolve();
        };
      });
    return queue.shift();
  }
  try {
    assert.equal((await next()).type, "connected");
    ws.send("{bad json");
    assert.equal((await next()).type, "error");
    ws.send("null");
    assert.equal((await next()).type, "error");
    ws.send(
      JSON.stringify({
        type: "create_session",
        candidate_name: "测试",
        question_total: 1,
      }),
    );
    assert.equal((await next()).type, "session");
    assert.equal((await next()).event_type, "QUESTION");
    assert.equal((await next()).type, "done");
  } finally {
    ws.close();
  }
});

test("rejects cross-origin websocket", async () => {
  const ws = open("https://untrusted.example");
  await new Promise((resolve) => {
    ws.on("error", resolve);
    ws.on("open", () => {
      ws.close();
      assert.fail("origin accepted");
    });
  });
});

test("serves the Web UI from the gateway", async () => {
  const response = await fetch(address + "/");
  assert.equal(response.status, 200);
  assert.match(await response.text(), /<title>AI Interview<\/title>/);
  assert.equal((await fetch(address + "/src/app.js")).status, 200);
  assert.equal((await (await fetch(address + "/healthz")).json()).status, "ok");
});

async function createSession() {
  const ws = open();
  return await new Promise((resolve, reject) => {
    const timeout = setTimeout(() => {
      ws.close();
      reject(Error("timeout"));
    }, 5000);
    let session;
    ws.on("error", reject);
    ws.on("message", (raw) => {
      const message = JSON.parse(raw);
      if (message.type === "connected")
        ws.send(
          JSON.stringify({
            type: "create_session",
            candidate_name: "SSE 测试",
            question_total: 1,
          }),
        );
      if (message.type === "session") session = message;
      if (message.type === "done") {
        clearTimeout(timeout);
        ws.close();
        resolve(session);
      }
    });
  });
}

test("answer SSE streams drafts and rejects oversized, invalid, unowned and cross-origin requests", async () => {
  const session = await createSession();
  const body = {
    session_id: session.id,
    session_token: session.session_token,
    text: "RAII 管理资源",
  };
  const post = (payload, origin = address) =>
    fetch(address + "/api/answer", {
      method: "POST",
      headers: { "Content-Type": "application/json", Origin: origin },
      body: JSON.stringify(payload),
    });
  assert.equal((await post({ ...body, session_token: "invalid" })).status, 403);
  assert.equal(
    (await post({ ...body, video_jpeg: "broken image" })).status,
    400,
  );
  assert.equal((await post(body, "https://untrusted.example")).status, 403);
  const oversized = await fetch(address + "/api/answer", {
    method: "POST",
    headers: { "Content-Type": "application/json", Origin: address },
    body: " ".repeat(2 * 1024 * 1024 + 1),
  });
  assert.equal(oversized.status, 413);
  const response = await post(body);
  assert.match(response.headers.get("content-type"), /text\/event-stream/);
  const events = (await response.text())
    .split("\n")
    .filter((line) => line.startsWith("data:"))
    .map((line) => JSON.parse(line.slice(5)));
  assert.ok(events.filter((event) => event.event_type === "DELTA").length > 1);
  assert.ok(events.some((event) => event.event_type === "FEEDBACK"));
  assert.ok(events.some((event) => event.event_type === "SUMMARY"));
  assert.equal(events.at(-1).type, "done");
});

test("report is returned by its independent authenticated SSE route", async () => {
  const session = await createSession();
  const answer = await fetch(address + "/api/answer", {
    method: "POST",
    headers: { "Content-Type": "application/json", Origin: address },
    body: JSON.stringify({
      session_id: session.id,
      session_token: session.session_token,
      text: "测试回答",
    }),
  });
  assert.equal(answer.status, 200);
  await answer.text();
  const report = await fetch(address + "/api/report", {
    method: "POST",
    headers: { "Content-Type": "application/json", Origin: address },
    body: JSON.stringify({
      session_id: session.id,
      session_token: session.session_token,
    }),
  });
  assert.equal(report.status, 200);
  const events = (await report.text())
    .split("\n")
    .filter((line) => line.startsWith("data:"))
    .map((line) => JSON.parse(line.slice(5)));
  assert.equal(
    events.find((event) => event.type === "report").report.overall_score,
    80,
  );
  assert.equal(events.at(-1).type, "done");
});
