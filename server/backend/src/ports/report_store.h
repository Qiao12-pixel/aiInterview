#pragma once
#include <nlohmann/json.hpp>
#include <optional>
#include <string>
namespace backend {
class ReportStore {
  public:
    virtual ~ReportStore() = default;
    virtual void Save(const std::string &id, const nlohmann::json &report) = 0;
    virtual std::optional<nlohmann::json> Load(const std::string &id) = 0;
    virtual nlohmann::json List(const std::string &owner, unsigned offset) = 0;
};
} // namespace backend
