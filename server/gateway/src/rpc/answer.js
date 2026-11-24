export function answer(client, context, data) {
  if (
    !context.state.sessionId ||
    typeof data.text !== "string" ||
    !data.text.trim()
  ) {
    context.finish({ details: "请先创建会话并输入回答" });
    return;
  }
  const call = client.sendText(
    { session_id: context.state.sessionId, text: data.text.trim() },
    context.options,
  );
  call.on("data", (event) =>
    context.send({ ...event, type: "event", event_type: event.type }),
  );
  call.on("error", context.finish);
  call.on("end", () => context.finish());
  return call;
}
