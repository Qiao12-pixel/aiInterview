export async function readJson(req, limit = 2 * 1024 * 1024) {
  const chunks = [];
  let size = 0;
  let oversized = false;
  for await (const chunk of req) {
    size += chunk.length;
    if (size > limit) oversized = true;
    else if (!oversized) chunks.push(chunk);
  }
  if (oversized) throw new Error("请求过大");
  return JSON.parse(Buffer.concat(chunks).toString());
}
