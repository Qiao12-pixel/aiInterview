export function report(client, context) {
  if (!context.state.sessionId) {
    context.finish({ details: "请先创建会话" });
    return;
  }
  return client.getReport(
    { session_id: context.state.sessionId },
    context.options,
    (error, result) => {
      if (error) {
        context.finish(error);
        return;
      }
      try {
        context.send({
          type: "report",
          report: JSON.parse(result.report_json),
        });
        context.finish();
      } catch {
        context.finish({ details: "服务端报告格式错误" });
      }
    },
  );
}
