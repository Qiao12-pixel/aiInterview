//
// Created by Joe on 26-4-3.
//

#ifndef LOGGER_H
#define LOGGER_H
#include <string>
#include <spdlog/spdlog.h>

/**
 * @file logger.h
 * @brief 日志系统模块
 * be
 *
 * 本文件定义了基于spdlog的日志封装类
 * 提供统一的日志接口，支持多级别日志，彩色输出，文本记录等功能
 *
 *
 * 核心功能
 * 1、多级别日志【TRACE, DEBUG, INFO, WARN, ERROR, CRITICAL】
 * 2、彩色控制台输出
 * 3、文件持久化【文件写入】
 * 4、格式化输出
 * 5、调试模式转换【开发/生产环境不同日志级别】
 */
namespace interview {
    namespace common {
        /**
         * @brief 封装spdlog日志库，提供统一的调用接口
         */
        class Logger {
        public:
            static void Init(const std::string& log_file = "../interview.log", bool debug_mode = false);

            /**
             * @brief 获取日志实例
             * 返回全局单例实例
             * 如果未初始化，会自动调用Init()使用默认配置
             * @return spdlog日志实例的共享指针
             */
            static std::shared_ptr<spdlog::logger>& GetLogger();
        private:
            static std::shared_ptr<spdlog::logger> logger_;
        };
        /**
         * 定义宏
         *
         */
        // ========== 便捷日志宏 ==========
        // 这些宏简化日志调用，自动获取日志实例
        // 使用示例：LOG_INFO("User {} logged in", username);

        /// @brief 记录TRACE级别日志（最详细的调试信息）
#define LOG_TRACE(...)    ::interview::common::Logger::GetLogger()->trace(__VA_ARGS__)

        /// @brief 记录DEBUG级别日志（调试信息）
#define LOG_DEBUG(...)    ::interview::common::Logger::GetLogger()->debug(__VA_ARGS__)

        /// @brief 记录INFO级别日志（一般信息）
#define LOG_INFO(...)     ::interview::common::Logger::GetLogger()->info(__VA_ARGS__)

        /// @brief 记录WARN级别日志（警告信息）
#define LOG_WARN(...)     ::interview::common::Logger::GetLogger()->warn(__VA_ARGS__)

        /// @brief 记录WARNING级别日志（同WARN）
#define LOG_WARNING(...)  ::interview::common::Logger::GetLogger()->warn(__VA_ARGS__)

        /// @brief 记录ERROR级别日志（错误信息）
#define LOG_ERROR(...)    ::interview::common::Logger::GetLogger()->error(__VA_ARGS__)

        /// @brief 记录CRITICAL级别日志（严重错误）
#define LOG_CRITICAL(...) ::interview::common::Logger::GetLogger()->critical(__VA_ARGS__)

    }
}


#endif //LOGGER_H
