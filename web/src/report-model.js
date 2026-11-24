// 将持久化数据转换为用户可读的展示模型，隐藏内部 ID 和技术字段。
const text = (value) => (typeof value === "string" ? value : "");
const list = (value) =>
  Array.isArray(value)
    ? value.filter((item) => typeof item === "string" && item.trim())
    : [];
const score = (value) =>
  typeof value === "number" &&
  Number.isFinite(value) &&
  value >= 0 &&
  value <= 100
    ? Math.round(value)
    : null;
export function reportView(report) {
  if (!report || typeof report !== "object" || Array.isArray(report))
    throw new Error("报告内容无效");
  const evaluation =
    report.evaluation && typeof report.evaluation === "object"
      ? report.evaluation
      : report;
  const records = Array.isArray(report.records)
    ? report.records
        .filter((record) => record && typeof record === "object")
        .map((record) => ({
          question: text(record.question),
          answer: text(record.answer),
          feedback: text(record.evaluation?.feedback) || text(record.feedback),
          score: score(record.evaluation?.score ?? record.score),
        }))
    : [];
  const total =
    Number.isInteger(report.total_questions) && report.total_questions > 0
      ? report.total_questions
      : null;
  return {
    candidate: text(report.candidate_name) || "候选人",
    records,
    total,
    summary: text(evaluation.summary) || text(report.summary),
    score: score(
      evaluation.overall_score ?? report.overall_score ?? report.average_score,
    ),
    sections: [
      ["技术优势", list(evaluation.strengths)],
      ["待提升方面", list(evaluation.weaknesses)],
      ["学习建议", list(evaluation.suggestions)],
    ].filter(([, items]) => items.length),
  };
}
