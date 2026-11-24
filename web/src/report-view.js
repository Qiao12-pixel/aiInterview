import { reportView } from "./report-model.js";

function element(tag, className, value) {
  const node = document.createElement(tag);
  if (className) node.className = className;
  if (value !== undefined) node.textContent = value;
  return node;
}
export function renderReport(container, messages, report) {
  const view = reportView(report);
  messages.reset();
  const header = element("section", "report-overview");
  header.setAttribute("aria-label", "面试概览");
  const title = element("div", "report-heading");
  title.append(
    element("strong", "", `${view.candidate}的面试回顾`),
    element("span", "report-readonly", "已完成 · 只读"),
  );
  header.append(
    title,
    element(
      "p",
      "",
      `${view.total ? `${view.total} 道主问题 · ` : ""}${view.records.length} 轮问答`,
    ),
  );
  if (view.score !== null)
    header.append(
      element("div", "report-score", `综合评分 ${view.score} / 100`),
    );
  container.append(header);
  view.records.forEach((record, index) => {
    if (record.question)
      messages.add(
        "assistant",
        record.question,
        `AI 面试官 · 第 ${index + 1} 轮`,
      );
    if (record.answer) messages.add("candidate", record.answer, view.candidate);
    if (record.feedback || record.score !== null) {
      messages.add(
        "assistant",
        record.feedback || "本轮未记录文字评价",
        `回答评价${record.score !== null ? ` · ${record.score} 分` : ""}`,
      );
    }
  });
  if (!view.records.length)
    messages.add("assistant", "这份历史报告没有保存逐轮问答记录。", "记录说明");
  if (view.summary) messages.add("assistant", view.summary, "面试总结");
  if (view.sections.length) {
    const sections = element("section", "report-details");
    sections.setAttribute("aria-label", "面试建议");
    for (const [title, items] of view.sections) {
      const section = element("div", "report-section");
      section.append(element("h3", "", title));
      const list = element("ul");
      items.forEach((item) => list.append(element("li", "", item)));
      section.append(list);
      sections.append(section);
    }
    container.append(sections);
  }
  container.scrollTop = 0;
  return view;
}
