#include <QApplication>
#include <QWidget>

#include "../include/common/config.h"
#include "../include/common/logger.h"
#include "../include/ui/MainWindow.h"
//
// Created by Joe on 26-4-1.
//
int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    QApplication::setApplicationName("C++ Interview System");
    QApplication::setApplicationVersion("1.0");
    QApplication::setOrganizationName("Interview");
    auto& cfg = interview::common::Config::Instance();
    cfg.LoadFromFile("/Users/joe/CLionProjects/aiInterview/config/default_config.json");
    LOG_INFO("api url {}", cfg.llm_config.api_url);
    interview::ui::MainWindow window;
    window.show();
    return app.exec();
}
/*#include "../include/common/config.h"
#include <spdlog/spdlog.h>
#include "../include/common/logger.h"
#include "../include/services/pdf_parser.h"
#include <string>
#include <iostream>
#include "../include/services/llm_client.h"*/

/*int main() {
    //配置文件测试+日志测试
    std::cout << "==================//配置文件测试+日志测试=========================" << std::endl;
    auto& config = interview::common::Config::Instance();
    config.LoadFromFile("/Users/joe/CLionProjects/aiInterview/config/default_config.json");
    SPDLOG_INFO("LLM api_url: {}", config.llm_config.api_url);
    LOG_INFO("LLM api_url = {}", config.llm_config.api_url);
    SPDLOG_INFO("LLM api_key: {}", config.llm_config.api_key.substr(0, 8));
    SPDLOG_INFO("Dialog bot_Name, {}", config.dialog_config.bot_name);
    //pdf读取测试
    std::cout << "==================//pdf读取测试=========================" << std::endl;
    interview::services::PDFParaser parser;
    std::string pdf_path = "/Users/joe/CLionProjects/aiInterview/resource/resume.pdf";
    LOG_INFO("Testing PDF{}", pdf_path);
    try {
        if (!parser.IsValidPDF(pdf_path)) {
            LOG_ERROR("PDF is invalid or cannot be loaded");
            return 1;
        }
        LOG_INFO("PDF is valid, start extracting text...");
        std::string text = parser.ExtractText(pdf_path);
        LOG_INFO("Extracted text length: {}", text.size());
        // LOG_INFO("Text preview: {}", text.substr(0, std::min<size_t>(text.size(), 300)));
        LOG_INFO("Text: {}", text);
        // return 0;
    } catch (const std::exception& e) {
        LOG_ERROR("PDF parsing failed :{}", e.what());
        return 1;
    }

    //LLM测试
    std::cout << "==================//LLM测试=========================" << std::endl;
    try {
        interview::services::LLMClient client;
        std::string reply = client.SendMessage("你好", "你是一名C++面试官");
        LOG_INFO("LLM reply: {}", reply);
        //return 0;
    } catch (const std::exception& e) {
        LOG_ERROR("Test failed: {}", e.what());
        return 1;
    }
    //LLM出题
    std::cout << "==================//LLM出题=========================" << std::endl;
    try {
        std::string text = parser.ExtractText(pdf_path);
        interview::services::LLMClient client;
        auto questions = client.GenerateQuestionFromResume(text, 10);
        LOG_INFO("questions {}", questions.dump(2));
        return 0;
    } catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
    }

}*/
