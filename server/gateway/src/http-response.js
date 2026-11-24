export function rejectJson(response, status, message) {
  response.writeHead(status, { "Content-Type": "application/json" });
  response.end(JSON.stringify({ message }));
  return true;
}
