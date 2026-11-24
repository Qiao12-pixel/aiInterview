import { readFile, stat } from "node:fs/promises";
import path from "node:path";

const mime = {
  ".html": "text/html; charset=utf-8",
  ".js": "text/javascript",
  ".css": "text/css",
};
export function createHttpHandler(webRoot, apiHandlers) {
  return async (req, res) => {
    if (req.url === "/healthz") {
      res.writeHead(200, { "Content-Type": "application/json" });
      res.end('{"status":"ok"}');
      return;
    }
    const apiHandler = apiHandlers[new URL(req.url, "http://localhost").pathname];
    if (apiHandler) {
      const expected = process.env.ALLOWED_ORIGIN || `http://${req.headers.host}`;
      if (req.headers.origin !== expected) {
        res.writeHead(403, { "Content-Type": "application/json" });
        res.end(JSON.stringify({ message: "Origin 不匹配" }));
        return;
      }
      if (await apiHandler(req, res)) return;
    }
    try {
      const pathname = decodeURIComponent(
        new URL(req.url, "http://localhost").pathname,
      );
      const file = path.resolve(
        webRoot,
        "." + (pathname === "/" ? "/index.html" : pathname),
      );
      if (!file.startsWith(webRoot + path.sep) || !(await stat(file)).isFile())
        throw new Error("not found");
      res.writeHead(200, {
        "Content-Type": mime[path.extname(file)] || "application/octet-stream",
        "X-Content-Type-Options": "nosniff",
      });
      res.end(await readFile(file));
    } catch {
      res.writeHead(404);
      res.end("Not found");
    }
  };
}
