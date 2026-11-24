const limit = 5 * 1024 * 1024;
export async function validateResume(file) {
  if (!file) return;
  if (!file.name.toLowerCase().endsWith(".pdf"))
    throw new Error("请选择 PDF 格式简历");
  if (!file.size || file.size > limit)
    throw new Error("简历不能为空，且不能超过 5 MiB");
  const header = new Uint8Array(await file.slice(0, 5).arrayBuffer());
  if (new TextDecoder("ascii").decode(header) !== "%PDF-")
    throw new Error("文件内容不是有效的 PDF");
}
export async function encodeResume(file) {
  if (!file) return new Uint8Array();
  await validateResume(file);
  return new Uint8Array(await file.arrayBuffer());
}
export function createResumeUpload() {
  const input = document.querySelector("#resumeFile"),
    status = document.querySelector("#resumeStatus"),
    clear = document.querySelector("#clearResume");
  let file = null,
    version = 0,
    checking = false,
    locked = false,
    validationError = "";
  function render() {
    input.disabled = locked || checking;
    clear.disabled = locked || (!file && !validationError);
  }
  input.onchange = async () => {
    const revision = ++version;
    const selected = input.files[0];
    file = null;
    checking = true;
    validationError = "";
    render();
    status.textContent = "正在检查简历…";
    status.classList.remove("invalid");
    try {
      await validateResume(selected);
      if (revision !== version) return;
      file = selected || null;
      status.textContent = file
        ? `${file.name} · ${(file.size / 1024).toFixed(0)} KB`
        : "可选 PDF，最大 5 MiB";
    } catch (error) {
      if (revision !== version) return;
      input.value = "";
      validationError = error.message;
      status.textContent = error.message;
      status.classList.add("invalid");
    } finally {
      if (revision === version) {
        checking = false;
        render();
      }
    }
  };
  clear.onclick = () => {
    version++;
    file = null;
    validationError = "";
    checking = false;
    input.value = "";
    status.textContent = "可选 PDF，最大 5 MiB";
    status.classList.remove("invalid");
    render();
  };
  return {
    setBusy(value) {
      locked = value;
      render();
    },
    async payload() {
      if (checking) throw new Error("请等待简历检查完成");
      if (validationError) throw new Error(validationError);
      return encodeResume(file);
    },
    get selected() {
      return Boolean(file);
    },
  };
}
