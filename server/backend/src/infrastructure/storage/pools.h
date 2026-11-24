#pragma once
#include "infrastructure/storage/connection_pool.h"
#include <nlohmann/json.hpp>
#include <mysql.h>
#include <hiredis/hiredis.h>
namespace backend::storage {
std::shared_ptr<Pool<MYSQL>> MysqlPool(const nlohmann::json &config);
std::shared_ptr<Pool<redisContext>> RedisPool(const nlohmann::json &config);
} // namespace backend::storage
