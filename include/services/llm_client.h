//
// Created by Joe on 26-4-2.
//

#ifndef LLM_CLIENT_H
#define LLM_CLIENT_H

#include <string>
#include <nlohmann/json.hpp>
#include <vector>

namespace interview {
    namespace services {
        /**
         * @brief LLM消息结构题
         * 表示LLM对话中的单条信息
         */
        struct Message {
            std::string role;///< 角色：system(系统提示), user(用户输入), assistant(AI回复)
            std::string content;
        };
        class LLMClient {
        public:

            LLMClient();//创建LLM客户端实例，初始化HTTP
            ~LLMClient();//清理HTTP资源
            /**
             * @brief 发送单条信息并获得回复
             * @param message 用户消息内容
             * @param system_prompt 系统提示，定义AI的行为和角色
             * @return LLM的回复文本
             */
            std::string SendMessage(const std::string& message, const std::string& system_prompt = "");

            /**
             * @brief 发送多轮对话并获取回复
             *
             * 工作流程：
             * 构建请求JSON
             * 使用libcurl发送POST请求LLM
             * 解析响应JSON
             * 提取assistant的回复文本
             * @param messages
             * @return LLM的回复文本
             */
            std::string SendConversation(const std::vector<Message>& messages);
            /**
             * @brief 根据简历提出问题
             */
            nlohmann::json GenerateQuestionFromResume(const std::string& resume_text, int min_question_num = 50);

            /**
             * @brief 评估候选人的回答
             * 使用LLM智能评估回答质量并决定是否追问。
             * 评分标准：
             * - 90-100分：深入准确，无需追问
             * - 70-89分：正确但不够深入
             * - 50-69分：基本理解但有遗漏
             * - 30-49分：理解不足
             * - 0-29分：答非所问或完全错误
             *
             * 追问决策：
             * - 分数40-85：需要追问以深入了解
             * - 其他：不追问
             *
             * 返回JSON格式：
             * {
             *   "score": 75,
             *   "feedback": "理解基本概念，但对底层实现掌握不够",
             *   "need_followup": true,
             *   "followup_question": "能详细说明shared_ptr的引用计数实现吗？",
             *   "strengths": ["理解了智能指针的基本用法"],
             *   "weaknesses": ["未提及线程安全问题"]
             * }
             */
            nlohmann::json EvaluateAnswer(const std::string& question, const std::string& answer);
            /**
             * @brief 生成面试总结和建议
             *
             * 分析整个面试过程，生成全面的总结报告。
             *
             * 总结内容：
             * 1. 综合评价候选人的技术水平
             * 2. 指出技术优势和薄弱环节
             * 3. 给出具体的学习建议
             * 4. 评估是否推荐录用
             * 5. 建议录用级别
             *
             * 返回JSON格式：
             * {
             *   "overall_score": 75,
             *   "summary": "候选人具备扎实的C++基础...",
             *   "strengths": ["内存管理理解深入", "熟悉STL容器"],
             *   "weaknesses": ["多线程经验不足"],
             *   "suggestions": ["系统学习C++11/14/17新特性"],
             *   "recommendation": "推荐",
             *   "hiring_level": "中级C++工程师"
             * }
             *
             * @param interview_records 面试记录JSON数组(所有问题和回答)
             * @param resume_text 简历文本(可选)，帮助LLM更好地评估
             * @return JSON对象，包含summary(总结)、suggestions(建议)等
             * @throws std::runtime_error LLM调用失败或JSON解析失败
            */
            nlohmann::json GenerateSummary(const nlohmann::json& interview_records, const std::string& resume_text = "");
        private:
            class LLMClientImpl;
            std::unique_ptr<LLMClientImpl> pimpl_;
        };
    }
}


#endif //LLM_CLIENT_H
