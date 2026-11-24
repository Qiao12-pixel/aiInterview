export function authorizeSession(registry, body) {
  if (!body || typeof body !== "object")
    return { status: 400, error: "无效 JSON 请求" };
  const session = registry.find(body.session_token, body.session_id);
  return session
    ? { session }
    : { status: 403, error: "会话已失效，请新建会话" };
}
