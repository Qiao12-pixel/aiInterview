#pragma once
#include <nlohmann/json.hpp>
namespace backend::database {
// 启动时初始化配置指定的数据库及应用表；保留已有数据。
void Initialize(const nlohmann::json &mysql);
} // namespace backend::database
