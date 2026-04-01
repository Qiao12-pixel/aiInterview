//
// Created by Joe on 26-4-1.
//
#include <spdlog/spdlog.h>
int main() {
    //简单使用
    spdlog::info("hello, {}", "spdlog");
    spdlog::warn("注意，这是一条警告信息");
    spdlog::error("错误代码: {}", 404);
    //设置全局日志级别
    spdlog::set_level(spdlog::level::debug);

    spdlog::trace("跟踪信息");//不会显示，因为trace < debug

    //下面全部会显示
    spdlog::debug("调试信息");
    spdlog::info("一般信息");
    spdlog::warn("警告信息");
    spdlog::error("错误信息");
    spdlog::critical("严重错误");

    //运行时修改级别
    spdlog::set_level(spdlog::level::warn);
    spdlog::info("这条信息不回显示"); //info < warn
    spdlog::error("这条错误信息会显示"); // error >= warn
    spdlog::set_level(spdlog::level::info);

    //自定义格式化
    spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [线程:%t] %v");
    spdlog::info("自定义格式的日志");
    spdlog::error("错误信息");

    //恢复默认格式
    spdlog::set_pattern("%+");
    spdlog::info("默认格式的日志");
    return 0;
}