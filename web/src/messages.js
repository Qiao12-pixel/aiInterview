export function createMessages(container) {
  const previews = new Map();
  const nearBottom = () =>
    container.scrollHeight - container.scrollTop - container.clientHeight < 80;
  function scroll(stick) {
    if (stick) container.scrollTop = container.scrollHeight;
  }
  function add(role, text, label) {
    const stick = nearBottom() || role === "candidate";
    if (container.querySelector(".empty")) container.replaceChildren();
    const el = document.createElement("div");
    el.className = `bubble ${role}`;
    const small = document.createElement("small");
    small.textContent =
      label ||
      (role === "candidate" ? "你" : role === "error" ? "提示" : "AI 面试官");
    const content = document.createElement("p");
    content.textContent = text;
    el.append(small, content);
    container.append(el);
    scroll(stick);
    return el;
  }
  return {
    add,
    capture() {
      return {
        nodes: Array.from(container.childNodes),
        scrollTop: container.scrollTop,
      };
    },
    restore(snapshot) {
      previews.clear();
      container.replaceChildren(...snapshot.nodes);
      container.scrollTop = snapshot.scrollTop;
    },
    reset() {
      container.replaceChildren();
      previews.clear();
    },
    preview(target, text) {
      let el = previews.get(target);
      const stick = nearBottom();
      if (!el) {
        el = add(
          "assistant",
          text,
          target === "SUMMARY"
            ? "面试总结"
            : target === "QUESTION"
              ? "AI 面试官"
              : "回答评价",
        );
        el.classList.add("streaming");
        previews.set(target, el);
      }
      el.querySelector("p").textContent = text;
      scroll(stick);
    },
    finish(target, text) {
      const el = previews.get(target),
        stick = nearBottom();
      if (el) {
        el.querySelector("p").textContent = text;
        el.classList.remove("streaming");
        previews.delete(target);
        scroll(stick);
      } else
        add(
          "assistant",
          text,
          target === "FEEDBACK"
            ? "回答评价"
            : target === "SUMMARY"
              ? "面试总结"
              : "AI 面试官",
        );
    },
    fail() {
      for (const el of previews.values()) {
        el.classList.remove("streaming");
        el.classList.add("error");
        el.querySelector("small").textContent = "未完成预览 · 未提交";
      }
      previews.clear();
    },
  };
}
