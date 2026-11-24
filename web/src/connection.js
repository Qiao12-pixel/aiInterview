// A health check, not a persistent socket. Each RPC owns its response stream.
export function createConnection({ onState }) {
  let ready = false,
    revision = 0;
  async function connect() {
    const current = ++revision;
    onState("connecting");
    try {
      const response = await fetch("/healthz", {
        cache: "no-store",
        signal: AbortSignal.timeout(5000),
      });
      if (!response.ok) throw Error();
      const health = await response.json();
      if (health.transport !== "grpc-web") throw Error();
      if (current === revision) {
        ready = true;
        onState("open");
      }
    } catch {
      if (current === revision) {
        ready = false;
        onState("error");
      }
    }
  }
  return {
    connect,
    open: () => ready,
    reset: connect,
    reconnect: connect,
    disconnect() {
      revision++;
      ready = false;
      onState("closed");
    },
  };
}
