//
// Created by Joe on 26-4-3.
//

#include "../../include/common/logger.h"
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <vector>
#include <memory>
#include <iostream>

namespace interview {
    namespace common {
        std::shared_ptr<spdlog::logger> Logger::logger_ = nullptr;//静态成员类内定义，类定初始化
        void Logger::Init(const std::string &log_file, bool debug_mode) {
            try {
                //创建多个sink [控制台 + 文件]
                std::vector<spdlog::sink_ptr> sinks;

                //彩色控制台输出
                auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
                console_sink->set_level(debug_mode ? spdlog::level::debug : spdlog::level::info);
                console_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] %v");
                sinks.push_back(console_sink);

                //文件输出
                auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(log_file, true);
                file_sink->set_level(spdlog::level::trace);
                file_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] [%s:%#] %v");
                sinks.push_back(file_sink);

                //创建logger
                logger_ = std::make_shared<spdlog::logger>("interview", sinks.begin(), sinks.end());
                logger_->set_level(debug_mode ? spdlog::level::debug : spdlog::level::info);
                logger_->flush_on(spdlog::level::warn);

                spdlog::set_default_logger(logger_);
                //LOG_INFO("Logger initialized - Debug mode: {}", debug_mode);
            } catch (spdlog::spdlog_ex& ex) {
                std::cerr << "Log initialization failed: " << ex.what() << std::endl;
            }
        }

        std::shared_ptr<spdlog::logger> &Logger::GetLogger() {
            if (!logger_) {
                Init();
                //spdlog::info("自定义logger初始化完成");
            }
            return logger_;
        }


    }
}