//
// Created by Joe on 26-4-2.
//

#include "../../include/common/config.h"
#include <fstream>
#include <stdexcept>

#include "../../include/services/llm_client.h"

namespace interview {
    namespace common {
        DialogConfig::DialogConfig() = default;
        LLMConfig::LLMConfig()  = default;

        Config &Config::Instance() {
            static Config instance;
            return instance;
        }
        Config::Config() = default;

        void Config::LoadFromFile(const std::string &path) {
            std::ifstream ifs(path);
            if (!ifs.is_open()) {
                throw std::runtime_error("Failed to open configuration file: " + path);
            }
            nlohmann::json root;
            try {
                ifs >> root;
            } catch (const std::exception& e) {
                throw std::runtime_error(std::string("Failed to parse cofiguration file: ") + e.what());
            }

            const auto& dialog = root.at("dialog");
            dialog_config.bot_name = dialog.at("bot_name").get<std::string>();
            dialog_config.system_role = dialog.at("system_role").get<std::string>();
            dialog_config.speaking_style = dialog.at("speaking_style").get<std::string>();

            const auto& llm = root.at("llm");
            llm_config.api_url = llm.at("api_url").get<std::string>();
            llm_config.api_key = llm.at("api_key").get<std::string>();
            llm_config.model = llm.at("model").get<std::string>();
            llm_config.temperature = llm.value("temperature", 0.3f);
            llm_config.max_tokens = llm.value("max_tokens", 4096);
            llm_config.timeout_seconds = llm.value("timeout_seconds", 60);

            ValidateConfiguration();
        }

        void Config::ValidateConfiguration() const {
            if (llm_config.api_url.empty()) {
                throw std::runtime_error("LLM api_url cannot be empty");
            } if (llm_config.api_key.empty()) {
                throw std::runtime_error("LLM api_key cannot be empty");
            } if (llm_config.model.empty()) {
                throw std::runtime_error("LLM model cannot be empty");
            } if (llm_config.temperature < 0.0f || llm_config.temperature > 2.0f) {
                throw std::runtime_error("LLM temperature must be between 0.0 and 2.0");
            } if (llm_config.max_tokens <= 0) {
                throw std::runtime_error("LLM max_tokens must be greater than 0");
            } if (llm_config.timeout_seconds <= 0) {
                throw std::runtime_error("LLM timeout_seconds must be greater than 0");
            }
        }




    }
}


