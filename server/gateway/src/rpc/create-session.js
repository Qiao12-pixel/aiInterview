export function createSession(client, context, data) {
  if (
    typeof data.candidate_name !== "string" ||
    !data.candidate_name.trim() ||
    !Number.isInteger(data.question_total) ||
    data.question_total < 1 ||
    data.question_total > 10
  ) {
    context.finish({ details: "请输入姓名并选择 1–10 道题" });
    return;
  }
  return client.createSession(
    {
      candidate_name: data.candidate_name.trim(),
      question_total: data.question_total,
    },
    context.options,
    (error, result) => {
      if (error) {
        context.finish(error);
        return;
      }
      try {
        const token = context.registry.issue(result.session.id);
        context.state.sessionId = result.session.id;
        context.send({
          ...result.session,
          type: "session",
          session_token: token,
        });
        context.send({
          ...result.first_question,
          type: "event",
          event_type: result.first_question.type,
        });
        context.finish();
      } catch {
        context.finish({ details: "网关会话容量已满" });
      }
    },
  );
}
