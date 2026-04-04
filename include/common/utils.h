//
// Created by Joe on 26-4-3.
//

#ifndef UTILS_H
#define UTILS_H
#include <string>
#include <fmt/core.h>
#include <nlohmann/json.hpp>
/**
 * @brief 生成UUID, JSON处理
 */





//json处理的内联函数
namespace interview {
    namespace common {
        inline std::string ExtractJSON(const std::string& response, char start_char, char end_char) {
            size_t json_start = response.find(start_char);
            size_t json_end  = response.rfind(end_char);
            if (json_start == std::string::npos || json_end == std::string::npos) {
                throw std::runtime_error("Respnese does't contain valid JSON markers");
            }
            return response.substr(json_start, json_end - json_start + 1);
        }
        inline nlohmann::json ParseJSONFromResponse(const std::string& response, char start_char = '{', char end_char = '}') {
            std::string json_str = ExtractJSON(response, start_char, end_char);
            return nlohmann::json::parse(json_str);

        }
    }
}



#endif //UTILS_H
