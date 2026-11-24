import test from "node:test";
import assert from "node:assert/strict";
import { encode, decode, concat } from "../src/rpc/protobuf.js";
import { call, frame } from "../src/rpc/grpc-web.js";
import { createSession } from "../src/rpc/interview.js";
const trailer = () => {
  const t = frame(
    new TextEncoder().encode("grpc-status: 0\r\ngrpc-message: \r\n"),
  );
  t[0] = 128;
  return t;
};
const original = globalThis.fetch;
function mock(bytes, cut = 1) {
  globalThis.fetch = async () =>
    new Response(
      new ReadableStream({
        start(controller) {
          for (let i = 0; i < bytes.length; i += cut)
            controller.enqueue(bytes.slice(i, i + cut));
          controller.close();
        },
      }),
      { headers: { "Content-Type": "application/grpc-web+proto" } },
    );
}
test("protobuf Unicode/binary and independent wire fixture", () => {
  assert.deepEqual(
    [...encode("AccountRequest", { operation: "me" })],
    [10, 2, 109, 101],
  );
  assert.deepEqual(
    decode(
      "InterviewEvent",
      encode("InterviewEvent", { type: 7, text: "中文🌟" }),
    ),
    { type: 7, text: "中文🌟" },
  );
  assert.deepEqual(
    [
      ...encode("CreateSessionRequest", {
        resume_pdf: Uint8Array.of(0, 255),
        question_total: 3,
      }),
    ],
    [18, 2, 0, 255, 24, 3],
  );
  assert.throws(
    () => decode("InterviewEvent", Uint8Array.of(26, 9, 1)),
    /Truncated/,
  );
});
test("fragmented binary stream including trailers and UTF8", async () => {
  try {
    const message = encode("InterviewEvent", { type: 1, text: "中文🌟" });
    mock(concat([frame(message), trailer()]));
    const events = [];
    await call(
      "StartInterview",
      "CreateSessionRequest",
      "InterviewEvent",
      { candidate_name: "Test", question_total: 1 },
      (v) => events.push(v),
    );
    assert.equal(events[0].text, "中文🌟");
  } finally {
    globalThis.fetch = original;
  }
});
test("session/event mapping retains existing UI contract", async () => {
  try {
    mock(
      concat([
        frame(
          encode("InterviewEvent", {
            type: 8,
            session_id: "abc",
            json_payload: JSON.stringify({
              id: "abc",
              candidate_name: "测试",
              question_total: 1,
            }),
          }),
        ),
        frame(encode("InterviewEvent", { type: 1, text: "首题" })),
        trailer(),
      ]),
    );
    const events = [];
    await createSession({ candidate_name: "测试", question_total: 1 }, (v) =>
      events.push(v),
    );
    assert.equal(events[0].id, "abc");
    assert.equal(events[1].event_type, "QUESTION");
    assert.equal(events[1].text, "首题");
    assert.equal(events[2].type, "done");
  } finally {
    globalThis.fetch = original;
  }
});
test("missing trailer and nonzero grpc status fail closed", async () => {
  try {
    mock(frame(encode("InterviewEvent", { type: 1, text: "partial" })));
    await assert.rejects(
      call("SendText", "TextMessage", "InterviewEvent", {}),
      /输出流中断/,
    );
    const fail = frame(
      new TextEncoder().encode("grpc-status: 16\r\ngrpc-message: expired\r\n"),
    );
    fail[0] = 128;
    mock(fail);
    await assert.rejects(
      call("SendText", "TextMessage", "InterviewEvent", {}),
      /expired/,
    );
  } finally {
    globalThis.fetch = original;
  }
});
