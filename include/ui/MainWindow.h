//
// Created by Joe on 26-4-1.
//

#ifndef MAINWINDOW_H
#define MAINWINDOW_H
#include <QMainWindow>
#include <QMenuBar>
#include <QMessageBox>
#include <QToolBar>
#include <QPushButton>
#include <QVBoxLayout>
#include <QTextEdit>
#include <QLabel>
#include <qlineedit.h>
#include <QProgressBar>
#include <QStatusBar>
#include <thread>

#include "../interview/interview_manager.h"

namespace interview {
    namespace ui {
        class MainWindow : public QMainWindow {
            Q_OBJECT
        public:
            explicit MainWindow(QWidget* parent = nullptr);
            ~MainWindow();

        private slots:
            void OnNewSession();
            void OnStartSession();
            void OnReplyMessage();
        private:
            void SetupUi();
            void SetupMenuBar();
            void SetupToolBar();
            void SetupCentralWidget();
            void AppendMessage(const QString& text, const QString& color = "black");

        private:
            std::thread m_session_thread_;
            std::unique_ptr<session::InterviewSession> m_interview_session_;
            bool m_intro_finished{false};
            bool m_interview_running_{false};

            QPushButton* m_btn_new_session_;
            QPushButton* m_btn_start_interview_;
            QPushButton* m_btn_stop_session_;
            QPushButton* m_btn_view_report_;
            /**
             * 文本回复按钮
             */
            QLineEdit* m_reply_msg_;
            QPushButton* m_btn_reply_msg_;
            QTextEdit* m_message_area_;
            QLabel* m_status_area_;

            QProgressBar* m_progress_bar_;
            /**
             * 会话窗口拿到的信息
             * 候选人姓名
             * 简历路径
             * 题目数量
             */
            QString candidate_name_;
            QString resume_path_;
            int min_question_;
        };
    }
}
#endif //MAINWINDOW_H
