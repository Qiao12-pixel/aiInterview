#pragma once
#include "interview/infrastructure/llm/config.h"
#include "interview/ports/model.h"
namespace backend::llm {
std::string Call(const Config &config, const nlohmann::json &body, const TokenSink &sink);
}
