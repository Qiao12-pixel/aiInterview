//
// Created by Joe on 26-4-2.
//
#include "../../include/interview/interview_manager.h"
#include "../../include/common/logger.h"
#include "../../include/services/llm_client.h"
#include "../../include/services/pdf_parser.h"

namespace interview {
    namespace session {
        struct InterviewRecord {
            std::string question;
            std::string category;
            std::string level;
            std::string answer;
            std::string timestamp;
            int score;
            std::string feedback;

            nlohmann::json ToJson() const {
                nlohmann::json j;
                j["question"] = question;
                j["category"] = category;
                j["level"] = level;
                j["answer"] = answer;
                j["timestamp"] = timestamp;
                j["score"] = score;
                j["feedback"] = feedback;
                return j;
            }
        };
        namespace {
            inline std::tm SafeLocalTime(std::time_t time) {
                std::tm tm{};
                localtime_r(&time, &tm);
                return tm;
            }
        }
        class InterviewSession::InterviewSessionImpl {
        public:

            std::string candidate_name;
            std::vector<InterviewRecord> records;
            int max_questions;
            int total_score;
            std::time_t start_time;

            std::string resume_text;
            nlohmann::json llm_questions;
            int curr_question_index;

            std::unique_ptr<services::LLMClient> llm_client;

            std::string followup_question;
            std::string summary_text;
            bool has_followed_up;

            InterviewSessionImpl(const std::string& name) : candidate_name(name), max_questions(15), total_score(0),
            curr_question_index(0), has_followed_up(false){
                start_time = std::time(nullptr);
                llm_client = std::make_unique<services::LLMClient>();
            }
            std::string GetCurrentTime() const {
                auto now = std::time(nullptr);
                auto tm = SafeLocalTime(now);
                std::ostringstream oss;
                oss << std::put_time(&tm, "%H:%M:%S");
                return oss.str();
            }
        };
        InterviewSession::InterviewSession(const std::string &candidate_name)
            :pimpl_(std::make_unique<InterviewSessionImpl>(candidate_name)){
            LOG_INFO("Interview session created for: {}", candidate_name);
        }
        InterviewSession::~InterviewSession() = default;

        void InterviewSession::GenerateDefaultQuestions(int num_questions) {
            try {
                LOG_INFO("Geneating {} default C++ interview questions using LLM...", num_questions);
                //生成默认问题
                std::string generic_resume = R"(C++开发工程师岗位，要求掌握：
C++基础(指针/引用/const)、内存管理(智能指针/RAII)、STL(容器/算法)、
面向对象(继承/多态/虚函数)、C++11/14/17新特性、多线程、设计模式)";
                pimpl_->llm_questions = pimpl_->llm_client->GenerateQuestionFromResume(generic_resume, num_questions);
                if (pimpl_->llm_questions.empty()) {
                    throw std::runtime_error("Failed to generate default questions");
                }
                pimpl_->curr_question_index = 0;
                pimpl_->max_questions = static_cast<int>(pimpl_->llm_questions.size());

                LOG_INFO("Generated {} default C++ questions", pimpl_->max_questions);

            } catch (const std::exception& e) {
                LOG_ERROR("Failed to generate default questions: {}", e.what());
                throw;
            }
        }
        void InterviewSession::GenerateQuestionsFromResume(const std::string &resume_pdf_path, int min_questions) {
            try {
                LOG_INFO("Loading resume from {}", resume_pdf_path);
                services::PDFParaser pdf_paraser;
                if (!pdf_paraser.IsValidPDF(resume_pdf_path)) {
                    throw std::runtime_error("Invalid PDF file: " + resume_pdf_path);
                }

                pimpl_->resume_text = pdf_paraser.ExtractText(resume_pdf_path);
                LOG_INFO("Resume text extracted {} characters", pimpl_->resume_text.length());

                //创建llm客户端
                if (!pimpl_->llm_client) {
                    pimpl_->llm_client = std::make_unique<services::LLMClient>();
                }
                //生成问题
                LOG_INFO("Generate question from resume using LLM...");
                pimpl_->llm_questions = pimpl_->llm_client->GenerateQuestionFromResume(pimpl_->resume_text, min_questions);
                if (pimpl_->llm_questions.empty()) {
                    throw std::runtime_error("Failed to generate questions from resume");
                }
                pimpl_->curr_question_index = 0;
                pimpl_->max_questions = static_cast<int>(pimpl_->llm_questions.size());
                LOG_INFO("Loaded {} questions from resume", pimpl_->max_questions);
            } catch (const std::exception& e) {
                LOG_ERROR("Failed to load questions from resume: {}", e.what());
                throw;
            }
        }

        std::string InterviewSession::GetIntroPrompt() const {
            return "现在开始C++技术面试。请你作为一位专业且友好的面试官，用简短温和的语气向候选人问好（比如'你好，欢迎参加今天的面试'），然后礼貌地邀请候选人做一个简单的自我介绍。记住要简短，不超过3句话。后续对应于面试者的回答都只回复(好的，我们继续)。";
            // return "你好，欢迎参加今天的 C++ 技术面试。下面我开始出题";
        }

        std::string InterviewSession::GetFirstQuestion() {
            if (!pimpl_->llm_questions.empty()) {
                pimpl_->curr_question_index = 0;
                auto& q = pimpl_->llm_questions[pimpl_->curr_question_index];
                std::string question_text = q["question"].get<std::string>();
                return "第一题：" + question_text;
            }
            throw std::runtime_error("Failed choose first question");

        }
        std::string InterviewSession::GetNextQuestion() {
            if (!pimpl_->llm_questions.empty()) {
                pimpl_->curr_question_index++;
                pimpl_->has_followed_up = false;

                if (pimpl_->curr_question_index >= pimpl_->max_questions) {
                    return "";//已经问完
                }
                auto& q = pimpl_->llm_questions[pimpl_->curr_question_index];
                std::string question_text = q["question"].get<std::string>();

                LOG_INFO("Moving to question index: {}", pimpl_->curr_question_index);
                return "第" + std::to_string(pimpl_->curr_question_index + 1) + "题：" + question_text;
            }
            throw std::runtime_error("No questions loaded. Call GenerateDefaultQuestions() or LoadQuestionsFromResume() first.");


        }
        int InterviewSession::GetCurrentQuestionIndex() const {
            return pimpl_->curr_question_index;
        }

        std::string InterviewSession::GetFollowUpQuestion() {
            if (!pimpl_->followup_question.empty()) {
                std::string question = pimpl_->followup_question;
                pimpl_->followup_question.clear();
                pimpl_->has_followed_up = true;
                return "追问: " + question;
            }
            return "";
        }
        void InterviewSession::RecordAnswer(const std::string &answer) {
            InterviewRecord record;
            record.answer = answer;
            record.timestamp = pimpl_->GetCurrentTime();

            if (!pimpl_->llm_questions.empty() && pimpl_->curr_question_index < pimpl_->max_questions) {
                auto& q = pimpl_->llm_questions[pimpl_->curr_question_index];
                bool is_followup = !pimpl_->followup_question.empty();
                if (!is_followup) {
                    record.question = q["question"].get<std::string>();
                } else {
                    record.question = "追问 - " + q["question"].get<std::string>();
                }
                record.category = q.value("category", "general");
                record.level = q.value("level", "intermediate");

                // 使用LLM评分（同时判断是否需要追问）
                try {
                    LOG_DEBUG("Evaluating answer with LLM...");
                    auto llm_eval = pimpl_->llm_client->EvaluateAnswer(record.question, answer);
                    record.score = llm_eval["score"].get<int>();
                    record.feedback = llm_eval["feedback"].get<std::string>();

                    if (!is_followup && !pimpl_->has_followed_up && llm_eval.value("need_followup", false)) {
                        if (llm_eval.contains("followup_question")) {
                            pimpl_->followup_question = llm_eval["followup_question"].get<std::string>();
                        }
                    }
                } catch (const std::exception& e) {
                    LOG_ERROR("LLM evaluation failed: {}", e.what());
                    record.score = 50;
                    record.feedback = "评分失败，给予中等分数";
                    pimpl_->followup_question.clear();
                }
            } else {
                LOG_WARNING("RecordAnswer called but no current question available");
                return;  // 无当前问题，不记录
            }
            pimpl_->records.push_back(record);
            pimpl_->total_score += record.score;

            LOG_INFO("Answer recorded - Score: {} ({}), Total records: {}",
                     record.score, record.feedback, pimpl_->records.size());

        }

        int InterviewSession::GetLastScore() const {
            if (pimpl_->records.empty()) {
                return 0;
            }
            return pimpl_->records.back().score;
        }

        bool InterviewSession::ShouldFollowUp() const {
            return !pimpl_->followup_question.empty();
        }

        bool InterviewSession::IsComplete() const {
            return (pimpl_->curr_question_index + 1) >= pimpl_->max_questions;
        }

    }
}