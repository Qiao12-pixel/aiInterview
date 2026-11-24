// Wire fields mirror proto/interview.proto. Unknown protobuf fields are skipped.
const encoder = new TextEncoder(),
  decoder = new TextDecoder("utf-8", { fatal: true });
export const schemas = {
  AccountRequest: {
    operation: [1, "s"],
    body_json: [2, "s"],
    source: [3, "s"],
  },
  AccountResponse: { result_json: [1, "s"] },
  CreateSessionRequest: {
    candidate_name: [1, "s"],
    resume_pdf: [2, "b"],
    question_total: [3, "n"],
  },
  TextMessage: { session_id: [1, "s"], text: [2, "s"] },
  InterviewEvent: {
    type: [1, "n"],
    session_id: [2, "s"],
    text: [3, "s"],
    json_payload: [4, "s"],
  },
  ReportRequest: { session_id: [1, "s"] },
  ReportResponse: { session_id: [1, "s"], report_json: [2, "s"] },
  ListReportsRequest: { offset: [1, "n"] },
  ListReportsResponse: { reports_json: [1, "s"] },
};
function varint(n) {
  const out = [];
  do {
    out.push(n % 128 | (n >= 128 ? 128 : 0));
    n = Math.floor(n / 128);
  } while (n);
  return out;
}
export function encode(schema, value) {
  const chunks = [];
  for (const [key, [id, type]] of Object.entries(schemas[schema])) {
    const v = value[key];
    if (v === undefined || v === null) continue;
    if (type === "n") {
      if (!Number.isSafeInteger(v) || v < 0 || v > 0xffffffff)
        throw Error("Invalid uint32");
      chunks.push(Uint8Array.from([...varint(id * 8), ...varint(v)]));
    } else {
      const bytes = type === "s" ? encoder.encode(v) : v;
      if (!(bytes instanceof Uint8Array)) throw Error("Invalid bytes");
      chunks.push(
        Uint8Array.from([...varint(id * 8 + 2), ...varint(bytes.length)]),
        bytes,
      );
    }
  }
  return concat(chunks);
}
export function concat(chunks) {
  const out = new Uint8Array(chunks.reduce((n, b) => n + b.length, 0));
  let pos = 0;
  for (const b of chunks) {
    out.set(b, pos);
    pos += b.length;
  }
  return out;
}
export function decode(schema, bytes) {
  let pos = 0;
  const out = {};
  const read = () => {
    let n = 0,
      m = 1;
    for (let i = 0; i < 10; i++) {
      if (pos >= bytes.length) throw Error("Truncated protobuf");
      const b = bytes[pos++];
      n += (b & 127) * m;
      if (!(b & 128)) return n;
      m *= 128;
    }
    throw Error("Invalid varint");
  };
  while (pos < bytes.length) {
    const tag = read(),
      wire = tag % 8,
      id = Math.floor(tag / 8);
    if (!id) throw Error("Invalid tag");
    const field = Object.entries(schemas[schema]).find(([, v]) => v[0] === id);
    let value;
    if (wire === 0) value = read();
    else if (wire === 2) {
      const n = read();
      if (n > bytes.length - pos) throw Error("Truncated field");
      value = bytes.subarray(pos, pos + n);
      pos += n;
    } else if (wire === 1) pos += 8;
    else if (wire === 5) pos += 4;
    else throw Error("Unsupported wire type");
    if (pos > bytes.length) throw Error("Truncated protobuf");
    if (field) {
      const [key, [, type]] = field;
      if ((type === "n" ? 0 : 2) !== wire) throw Error("Wrong wire type");
      out[key] = type === "s" ? decoder.decode(value) : value;
    }
  }
  return out;
}
