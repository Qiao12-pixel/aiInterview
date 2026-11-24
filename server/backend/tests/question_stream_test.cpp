#include "application/engine.h"
#include "infrastructure/llm.h"
#include <stdexcept>
#include <vector>
int main() {
    backend::Engine engine(backend::MakeMockModel());
    std::vector<backend::Event> events;
    const auto session = engine.Create(
        "test", 2, "", "", [&](const backend::Event &event) { events.push_back(event); });
    if (events.size() < 2 || events.back().text != session->current)
        return 1;
    for (const auto &event : events)
        if (event.type != "DELTA" || event.payload["target"] != "QUESTION")
            return 2;
    backend::Engine invalid([](const std::string &, const backend::Json &,
                               const backend::TokenSink &sink) -> backend::Json {
        if (sink)
            sink("{\"questions\":[{\"question\":\"预览题目\"}");
        return backend::Json::array();
    });
    bool previewed = false;
    try {
        invalid.Create("test", 1, "", "", [&](const backend::Event &) { previewed = true; });
        return 3;
    } catch (const std::runtime_error &) {
    }
    return previewed ? 0 : 4;
}
