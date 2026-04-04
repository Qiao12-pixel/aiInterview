//
// Created by Joe on 26-4-2.
//

#ifndef CONFIG_H
#define CONFIG_H
#include <string>
#include <nlohmann/json.hpp>

namespace interview {
    namespace common {
        struct DialogConfig {
            std::string bot_name;
            std::string system_role;
            std::string speaking_style;
            std::string city;
            bool strict_audit = false;
            std::string audit_response;
            int recv_timeout = 0;
            std::string input_mode;

            DialogConfig();
        };

        struct LLMConfig {
            std::string api_url;
            std::string api_key;
            std::string model;
            float temperature = 0.3f;
            int max_tokens = 4096;
            int timeout_seconds = 60;

            LLMConfig();
        };
        class Config {
        public:
            static Config& Instance();
            DialogConfig dialog_config;
            LLMConfig llm_config;
            void LoadFromFile(const std::string& path);
            void ValidateConfiguration() const;

        private:
            Config();//私有构造，防止外部直接创建实例，统一用单例模式
            Config(const Config&) = delete;
            Config& operator=(const Config&) = delete;
        };
    }
}



#endif //CONFIG_H
