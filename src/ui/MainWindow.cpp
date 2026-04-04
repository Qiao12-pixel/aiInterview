//
// Created by Joe on 26-4-1.
//
#include "../../include/ui/MainWindow.h"

#include <iostream>
#include <thread>

#include "../../include/ui/config_dialog.h"

namespace interview {
    namespace ui {
        MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), candidate_name_("候选人"), min_question_(15){
            setMenuBar(new QMenuBar(this));
            SetupUi();
            SetupMenuBar();
            SetupToolBar();
            SetupCentralWidget();

        }
        MainWindow::~MainWindow() {

        }

        void MainWindow::SetupUi() {
            setWindowTitle("AI智能面试系统");
            // setMinimumSize(QSize(800, 600));


            QWidget* central = new QWidget(this);
            setCentralWidget(central);
        }
        void MainWindow::SetupMenuBar() {
            // QMenuBar* menu_bar = new QMenuBar(this);
            // setMenuBar(menu_bar);
            QMenuBar* menu_bar = menuBar();
            menu_bar->setVisible(true);

            //文件菜单
            QMenu* file_menu = menu_bar->addMenu("文件(&F)");
            QAction* new_action = file_menu->addAction("新建会话(&N)");
            new_action->setShortcut(QKeySequence::New);
            connect(new_action, &QAction::triggered, this, &MainWindow::OnNewSession);

            file_menu->addSeparator();

            QAction* exit_action = file_menu->addAction("退出(&X)");
            exit_action->setShortcut(QKeySequence::Quit);
            connect(exit_action, &QAction::triggered, this, &QWidget::close);

            QMenu* help_menu = menu_bar->addMenu("帮助(&H)");
            QAction* about_action = help_menu->addAction("关于(&A)");
            connect(about_action, &QAction::triggered, this, [this]() {
               QMessageBox::about(this, "关于", "C++智能面试系统 v1.0\n"
                                              "基于AI的实时语音技术面试平台\n"
                                              "支持建立驱动问题生成与智能评分");
            });
        }
        void MainWindow::SetupToolBar() {
            QToolBar* tool_bar = addToolBar("主工具栏");
            tool_bar->setMovable(false);

            m_btn_new_session_ = new QPushButton("新建会话", this);
            m_btn_new_session_->setDefault(true);
            m_btn_start_interview_ = new QPushButton("开始面试", this);
            m_btn_stop_session_ = new QPushButton("停止面试", this);
            m_btn_view_report_ = new QPushButton("查看报告", this);
            tool_bar->addWidget(m_btn_new_session_);
            tool_bar->addSeparator();
            tool_bar->addWidget(m_btn_start_interview_);
            tool_bar->addSeparator();
            tool_bar->addWidget(m_btn_stop_session_);
            tool_bar->addSeparator();
            tool_bar->addWidget(m_btn_view_report_);
            tool_bar->addSeparator();

            connect(m_btn_new_session_, &QPushButton::clicked, this, &MainWindow::OnNewSession);
            connect(m_btn_start_interview_, &QPushButton::clicked, this, &MainWindow::OnStartSession);

        }
        void MainWindow::SetupCentralWidget() {
            QWidget* central = centralWidget();
            QVBoxLayout* layout = new QVBoxLayout(central);

            //状态栏
            m_status_area_ = new QLabel("就绪", this);
            m_status_area_->setStyleSheet(
                "QLabel { "
                "  padding: 8px 12px; "
                "  font-weight: bold; "
                "  font-size: 12pt; "
                "  background-color: #f5f5f5; "
                "  border: 1px solid #ccc; "
                "  color: #333; "
                "}"
            );
            layout->addWidget(m_status_area_);

            //消息显示区域
            m_message_area_ = new QTextEdit(this);
            m_message_area_->setReadOnly(true);
            m_message_area_->setLineWrapMode(QTextEdit::WidgetWidth);
            m_message_area_->setWordWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
            m_message_area_->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
            m_message_area_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
            m_message_area_->setStyleSheet(
                "QTextEdit { "
                "  background-color: #ffffff; "
                "  border: 1px solid #ddd; "
                "  padding: 10px; "
                "  font-family: 'Microsoft YaHei', sans-serif; "
                "  font-size: 16pt; "
                "  line-height: 1.6; "
                "}"
                "QScrollBar:vertical {"
                "  border: none;"
                "  background: #f0f0f0;"
                "  width: 10px;"
                "  margin: 0px;"
                "}"
                "QScrollBar::handle:vertical {"
                "  background: #c0c0c0;"
                "  border-radius: 5px;"
                "  min-height: 20px;"
                "}"
                "QScrollBar::handle:vertical:hover {"
                "  background: #999;"
                "}"
                "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {"
                "  height: 0px;"
                "}"
            );
            layout->addWidget(m_message_area_);
            //进度条
            m_progress_bar_ = new QProgressBar(this);
            m_progress_bar_->setRange(0, 10);
            m_progress_bar_->setValue(0);
            m_progress_bar_->setTextVisible(true);
            m_progress_bar_->setFormat("进度: %v/%m 题");
            m_progress_bar_->setMinimumHeight(28);
            m_progress_bar_->setStyleSheet(
                "QProgressBar {"
                "  border: 1px solid #ddd;"
                "  background-color: #f0f0f0;"
                "  text-align: center;"
                "  font-weight: bold;"
                "  font-size: 10pt;"
                "  color: #333;"
                "}"
                "QProgressBar::chunk {"
                "  background-color: #4CAF50;"
                "}"
            );
            layout->addWidget(m_progress_bar_);

            QHBoxLayout* send_layout = new QHBoxLayout();
            m_btn_reply_msg_ = new QPushButton("提交回答", this);
            m_reply_msg_ = new QLineEdit(this);
            send_layout->addWidget(m_reply_msg_);
            send_layout->addWidget(m_btn_reply_msg_);
            layout->addLayout(send_layout);
            connect(m_btn_reply_msg_, &QPushButton::clicked, this, &MainWindow::OnReplyMessage);
            //状态栏--左下角小字
            statusBar()->showMessage("就绪");
        }
        void MainWindow::OnNewSession() {
            ConfigDialog dialog(this);
            if (dialog.exec() == QDialog::Accepted) {
                candidate_name_ = dialog.GetCandidateName();
                resume_path_ = dialog.GetResumePath();
                min_question_ = dialog.GetMinQuestions();

                m_message_area_->clear();
                m_progress_bar_->setValue(0);
                m_progress_bar_->setMaximum(min_question_);

                // 配置信息显示
                QString config_info = QString("===== 会话配置 =====\n")
                                     + QString("候选人: %1\n").arg(candidate_name_);
                if (!resume_path_.isEmpty()) {
                    config_info += QString("简历: %1\n").arg(resume_path_);
                }
                config_info += QString("问题数量: %1 题\n").arg(min_question_);
                config_info += QString("========================\n");

                AppendMessage(config_info);

                m_btn_start_interview_->setEnabled(true);
                statusBar()->showMessage("会话已配置，点击'开始面试'启动");
                m_status_area_->setText("会话已配置，点击'开始面试'启动""");
            }
        }
        void MainWindow::OnStartSession() {
            if (candidate_name_.isEmpty()) {
                QMessageBox::warning(this, "警告", "请先新建会话进行上传信息");
                return;
            }

            m_btn_start_interview_->setEnabled(false);
            m_btn_new_session_->setEnabled(false);

            //根据是否有简历显示不同状态
            //现在默认是必须上传简历的
            if (!resume_path_.isEmpty()) {
                m_status_area_->setText("● 解析简历，生成问题中");
                m_status_area_->setStyleSheet(
                "QLabel { "
                            "  padding: 8px 12px; "
                            "  font-weight: bold; "
                            "  font-size: 12pt; "
                            "  background-color: #fff3e0; "
                            "  border: 1px solid #ffb74d; "
                            "  border-radius: 4px; "
                            "  color: #ff9800; "
                            "}"
                );
                AppendMessage("正在解析简历，生成问题中...\n", "blue");
                statusBar()->showMessage("正在解析简历，生成问题中...");
            } else {
                m_status_area_->setText("● 未上传简历生成默认问题中...");
                m_status_area_->setStyleSheet(
                "QLabel { "
                            "  padding: 8px 12px; "
                            "  font-weight: bold; "
                            "  font-size: 12pt; "
                            "  background-color: #fff3e0; "
                            "  border: 1px solid #ffb74d; "
                            "  border-radius: 4px; "
                            "  color: #ff9800; "
                            "}"
                );
                AppendMessage("正在生成问题中...\n", "blue");
                statusBar()->showMessage("正在生成问题中...");
            }

            //生成问题
            try {
                m_interview_session_ = std::make_unique<session::InterviewSession>(candidate_name_.toStdString());

                // 根据简历生成题目
                m_interview_session_->GenerateQuestionsFromResume(resume_path_.toStdString(), min_question_);

                //先显示开场白、再显示第一题
                const std::string intro = m_interview_session_->GetIntroPrompt();
                const std::string first_question = m_interview_session_->GetFirstQuestion();
                AppendMessage(QString("面试官: %1").arg(QString::fromStdString(intro)), "#0066cc");
                AppendMessage(QString("面试官: %1").arg(QString::fromStdString(first_question)), "#0066cc");

                m_intro_finished = true;
                m_interview_running_ = true;

                m_progress_bar_->setMaximum(min_question_);
                m_progress_bar_->setValue(1);

                m_status_area_->setText("● 面试进行中");
                m_status_area_->setStyleSheet(
                "QLabel { "
                            "  padding: 8px 12px; "
                            "  font-weight: bold; "
                            "  font-size: 12pt; "
                            "  background-color: #e8f5e9; "
                            "  border: 1px solid #81c784; "
                            "  border-radius: 4px; "
                            "  color: #2e7d32; "
                            "}"
                );
                statusBar()->showMessage("面试进行中");

            } catch (const std::exception& e) {
                QMessageBox::critical(this, "错误", QString::fromStdString(e.what()));

                m_status_area_->setText("● 启动失败");
                statusBar()->showMessage("启动失败");
            }



        }
        void MainWindow::OnReplyMessage() {
            if (!m_interview_running_ || !m_interview_session_) {
                QMessageBox::information(this, "提示", "请先开始面试");
                return;
            }
            const QString reply_text = m_reply_msg_->text().trimmed();//去掉字符串 开头 和 结尾 的所有空白字符
            if (reply_text.isEmpty()) {
                return;
            }
            AppendMessage(candidate_name_ + ": " + reply_text, "red");
            m_reply_msg_->clear();

            try {
                m_interview_session_->RecordAnswer(reply_text.toStdString());
                if (m_interview_session_->ShouldFollowUp()) {
                    const std::string followup = m_interview_session_->GetFollowUpQuestion();
                    AppendMessage(QString("面试官: %1").arg(QString::fromStdString(followup)), "#0066cc");
                    return;
                }
                /*if (m_interview_session_->IsComplete()) {
                    const std::string summary = m_interview_session_->GenerateSummary();
                    AppendMessage(QString("面试总结: %1").arg(QString::fromStdString(summary)), "green");

                    m_interview_running_ = false;
                    m_status_area_->setText("面试已结束");
                    statusBar()->showMessage("面试已结束");
                    return;
                }*/

                const std::string next_question = m_interview_session_->GetNextQuestion();
                AppendMessage(QString("面试官: %1").arg(QString::fromStdString(next_question)), "blue");

                m_progress_bar_->setValue(m_interview_session_->GetCurrentQuestionIndex() + 1);

            } catch (const std::exception& e) {
                QMessageBox::critical(this, "错误", QString::fromStdString(e.what()));
                m_interview_running_ = false;
                m_status_area_->setText("面试已结束");
                statusBar()->showMessage("面试已结束");
            }
        }


        void MainWindow::AppendMessage(const QString& text, const QString& color) {
            QString html_text = QString("<span style='color: %1;'>%2</span><br>")
                        .arg(color)
                        .arg(text.toHtmlEscaped().replace("\n", "<br>"));
            m_message_area_->moveCursor(QTextCursor::End);
            m_message_area_->insertHtml(html_text);
            m_message_area_->moveCursor(QTextCursor::End);
            m_message_area_->ensureCursorVisible();
        }

    }
}

