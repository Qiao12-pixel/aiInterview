#pragma once
#include "ports/model.h"
namespace backend {
Model MakeMockModel();
Model MakeModel(const std::string &config_path, bool mock);
} // namespace backend
