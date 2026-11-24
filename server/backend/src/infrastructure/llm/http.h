#pragma once
#include "infrastructure/llm/config.h"
#include "ports/model.h"
namespace backend::llm {
std::string Call(const Config &config, const nlohmann::json &body, const TokenSink &sink);
}
