//
// Created by Joe on 26-4-2.
//

#ifndef INTERVIEW_MANAGER_H
#define INTERVIEW_MANAGER_H

#include <memory>
#include <string>
#include <nlohmann/json.hpp>
namespace interview {
    namespace session {
        class InterviewSession {
        public:
            InterviewSession(const std::string& candidate_name = "候选人");
            ~InterviewSession();

            //生成默认问题
            void GenerateDefaultQuestions(int num_questions = 15);
            //生成简历驱动的问题
            void GenerateQuestionsFromResume(const std::string& resume_pdf_path, int min_questions = 15);

            /**
             * @brief 获取开场白提示词
             * @return
             */
            std::string GetIntroPrompt() const;
            //获取第一个问题
            std::string GetFirstQuestion();
            //获取下一个问题
            std::string GetNextQuestion();
            //获取追问问题
            std::string GetFollowUpQuestion();
            /**
             * @brief 记录候选人的回答
             *
             * 保存回答并使用LLM评分（如果启用）。
             * 评分结果包括分数、反馈、是否需要追问等。
             *
             * 工作流程：
             * 1. 获取当前问题信息
             * 2. 创建面试记录
             * 3. 调用LLM评估回答（如果启用）
             * 4. 保存评分和反馈
             * 5. 判断是否需要追问
             * 6. 累计总分
             *
             * @param answer 候选人的回答文本（ASR识别结果）
             */
            void RecordAnswer(const std::string& answer);
            /**
             * @brief 获取最后一次回答的分数
             *
             * @return 最后一次回答的分数（0-100），如果无记录则返回0
             */
            int GetLastScore() const;
            /**
     * @brief 判断是否需要追问
     *
     * 基于LLM评估结果判断是否应该对上一次回答进行追问。
     *
     * @return true=需要追问，false=不需要追问
     */
            bool ShouldFollowUp() const;

            /**
             * @brief 检查面试是否已完成
             *
             * 当所有问题都已提问完毕时返回true。
             *
             * @return true=所有问题已问完，false=还有问题未提问
             */
            bool IsComplete() const;

            /**
             * @brief 生成面试总结
             *
             * 使用LLM分析所有面试记录，生成详细的面试总结。
             * 包括整体评价、技术优势、薄弱环节、学习建议和录用建议。
             *
             * @return 面试总结文本（中文自然语言）
             */
            std::string GenerateSummary() const;
            /**
             * @brief 生成面试报告JSON
             *
             * 将所有面试数据整理成结构化的JSON报告。
             *
             * JSON结构：
             * - candidate_name: 候选人姓名
             * - interview_date: 面试日期时间
             * - duration_minutes: 面试时长（分钟）
             * - total_questions: 问题总数
             * - average_score: 平均分
             * - total_score: 总分
             * - records: 每个问题的详细记录数组
             * - evaluation: 综合评价
             *
             * @return 面试报告JSON对象
             */
            nlohmann::json GenerateReport() const;
            /**
             * @brief 保存面试报告到文件
             *
             * 将报告保存为格式化的JSON文件。
             *
             * @param filename 输出文件名，默认为空（自动生成：interview_report_YYYYMMDD_HHMMSS.json）
             * @return 实际保存的文件路径
             * @throws std::runtime_error 文件写入失败
             */
            std::string SaveReport(const std::string& filename = "") const;

            int GetCurrentQuestionIndex() const;
        private:
            class InterviewSessionImpl;
            std::unique_ptr<InterviewSessionImpl> pimpl_;

        };
    }
}
#endif //INTERVIEW_MANAGER_H
