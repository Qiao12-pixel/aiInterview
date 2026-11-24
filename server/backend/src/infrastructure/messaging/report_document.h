#pragma once
#include <nlohmann/json.hpp>
namespace backend::messaging {
std::string RenderReportDocument(const nlohmann::json &report);
}
