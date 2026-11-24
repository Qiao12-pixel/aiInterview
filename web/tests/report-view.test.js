import test from "node:test";
import assert from "node:assert/strict";
import { reportView } from "../src/report-model.js";
import { renderReport } from "../src/report-view.js";
import { createMessages } from "../src/messages.js";

// Minimal DOM exercising the rendering contract without a browser dependency.
class Node {
  constructor(tag) {
    this.tag = tag;
    this.children = [];
    this.className = "";
    this.attributes = {};
    this.value = "";
    this.scrollTop = 0;
    this.clientHeight = 400;
    this.scrollHeight = 400;
    this.text = "";
    this.classList = {
      add: (name) => {
        this.className += " " + name;
      },
      remove: (name) => {
        this.className = this.className
          .split(" ")
          .filter((item) => item !== name)
          .join(" ");
      },
    };
  }
  set textContent(value) {
    this.text = String(value);
    this.children = [];
  }
  get textContent() {
    return this.text + this.children.map((child) => child.textContent).join("");
  }
  get childNodes() {
    return this.children;
  }
  append(...nodes) {
    this.children.push(...nodes);
  }
  replaceChildren(...nodes) {
    this.text = "";
    this.children = [...nodes];
  }
  setAttribute(key, value) {
    this.attributes[key] = value;
  }
  querySelector(selector) {
    return (
      this.walk().find((node) =>
        selector.startsWith(".")
          ? node.className.split(" ").includes(selector.slice(1))
          : node.tag === selector,
      ) || null
    );
  }
  walk() {
    return this.children.flatMap((child) => [child, ...child.walk()]);
  }
}
globalThis.document = { createElement: (tag) => new Node(tag) };
const report = {
  candidate_name: "候选人",
  session_id: "internal-id",
  owner_user_id: "private-owner",
  total_questions: 1,
  overall_score: 79,
  summary: "理解基本原理，建议补充实现细节。",
  strengths: ["理解 RAII"],
  weaknesses: ["同步机制有遗漏"],
  suggestions: ["补充项目实例"],
  records: [
    {
      question: "主问题？",
      answer: "第一轮回答",
      evaluation: {
        score: 65,
        feedback: "请补充细节",
        need_followup: true,
        followup_question: "追问？",
      },
    },
    {
      question: "追问？",
      answer: "第二轮回答",
      evaluation: { score: 93, feedback: "解释充分" },
    },
  ],
};

test("history replays question, answer and evaluation in order without raw JSON or private identifiers", () => {
  const container = new Node("div"),
    messages = createMessages(container);
  messages.add("assistant", "old conversation");
  const view = renderReport(container, messages, report);
  assert.equal(view.records.length, 2);
  assert.equal(container.scrollTop, 0);
  const bubbles = container.children.filter((node) =>
    node.className.includes("bubble"),
  );
  assert.deepEqual(
    bubbles.map((node) => node.querySelector("p").textContent),
    [
      "主问题？",
      "第一轮回答",
      "请补充细节",
      "追问？",
      "第二轮回答",
      "解释充分",
      "理解基本原理，建议补充实现细节。",
    ],
  );
  assert.match(container.textContent, /综合评分 79/);
  assert.match(container.textContent, /93 分/);
  assert.match(container.textContent, /技术优势/);
  assert.doesNotMatch(
    container.textContent,
    /owner_user_id|private-owner|internal-id|old conversation/,
  );
  assert.equal(
    bubbles.filter((node) => node.querySelector("p").textContent === "追问？")
      .length,
    1,
  );
});
test("live conversation snapshot preserves original nodes and scroll position", () => {
  const container = new Node("div"),
    messages = createMessages(container);
  const original = messages.add("candidate", "正在输入前的对话");
  container.scrollTop = 72;
  const snapshot = messages.capture();
  renderReport(container, messages, report);
  messages.restore(snapshot);
  assert.equal(container.children[0], original);
  assert.equal(container.scrollTop, 72);
  assert.equal(container.children.length, 1);
});
test("legacy feedback/score fields and nested evaluation are readable", () => {
  const view = reportView({
    records: [{ question: "Q", answer: "A", score: 0, feedback: "旧评价" }],
    evaluation: {
      summary: "旧总结",
      overall_score: 0,
      suggestions: ["旧建议"],
    },
  });
  assert.equal(view.records[0].score, 0);
  assert.equal(view.score, 0);
  assert.equal(view.summary, "旧总结");
  const container = new Node("div");
  renderReport(container, createMessages(container), { summary: "只有总结" });
  assert.match(container.textContent, /没有保存逐轮问答记录/);
  assert.match(container.textContent, /只有总结/);
});
test("untrusted text stays text, malformed records do not create markup or technical dumps", () => {
  const payload = "<img src=x onerror=alert(1)>";
  const container = new Node("div");
  renderReport(container, createMessages(container), {
    candidate_name: payload,
    records: [
      null,
      {
        question: payload,
        answer: payload,
        evaluation: { score: 500, feedback: payload },
      },
    ],
    strengths: [{}, payload],
  });
  assert.equal(container.walk().filter((node) => node.tag === "img").length, 0);
  assert.match(container.textContent, /<img src=x/);
  assert.equal(reportView({ overall_score: 500 }).score, null);
  assert.throws(() => reportView(null), /报告内容无效/);
});
