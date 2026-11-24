#include "application/engine.h"
#include "infrastructure/llm.h"
#include <stdexcept>
struct Store : backend::ReportStore {
    bool fail = true;
    std::optional<nlohmann::json> saved;
    void Save(const std::string &, const nlohmann::json &report) override {
        if (fail)
            throw std::runtime_error("db down");
        saved = report;
    }
    std::optional<nlohmann::json> Load(const std::string &) override { return saved; }
    nlohmann::json List(const std::string &, unsigned) override { return nlohmann::json::array(); }
};
int main() {
    auto store = std::make_shared<Store>();
    backend::Engine engine(backend::MakeMockModel(), store);
    auto s = engine.Create("test", 1);
    try {
        engine.Answer(*s, "answer");
        return 1;
    } catch (const std::runtime_error &) {
    }
    if (s->completed || !s->records.empty())
        return 2;
    store->fail = false;
    engine.Answer(*s, "answer");
    if (!s->completed || !engine.Report(s->id))
        return 3;
}
