#include "ui/MainWindow.h"
#include "ui/config_dialog.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTimer>
namespace interview::ui {
void MainWindow::SetupCentralWidget() {
    QWidget *central = centralWidget();
    QVBoxLayout *layout = new QVBoxLayout(central);
    layout->setContentsMargins(20, 16, 20, 16);
    layout->setSpacing(12);

    // 状态栏
    m_status_area_ = new QLabel("● 就绪  ·  创建会话后开始", this);
    m_status_area_->setStyleSheet("QLabel { "
                                  "  padding: 8px 12px; "
                                  "  font-weight: bold; "
                                  "  font-size: 12pt; "
                                  "  background-color: #eef4ff; "
                                  "  border: 1px solid #c8d9f5; "
                                  "  border-radius: 8px; "
                                  "  color: #24446f; "
                                  "}");
    layout->addWidget(m_status_area_);

    auto *body_layout = new QHBoxLayout();
    body_layout->setSpacing(12);

    auto *conversation_card = new QFrame(this);
    conversation_card->setObjectName("conversationCard");
    auto *conversation_layout = new QVBoxLayout(conversation_card);
    auto *conversation_title = new QLabel("面试对话", conversation_card);
    conversation_title->setStyleSheet("font-size: 15pt; font-weight: 700; color: #162b4d;");
    conversation_layout->addWidget(conversation_title);

    m_message_area_ = new QTextEdit(conversation_card);
    m_message_area_->setReadOnly(true);
    m_message_area_->setLineWrapMode(QTextEdit::WidgetWidth);
    m_message_area_->setWordWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
    m_message_area_->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_message_area_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_message_area_->setStyleSheet(
        "QTextEdit { "
        "  background-color: #ffffff; border: 0; padding: 10px; "
        "  font-family: -apple-system, 'PingFang SC', sans-serif; font-size: 13pt; "
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
        "}");
    conversation_layout->addWidget(m_message_area_);
    body_layout->addWidget(conversation_card, 7);

    auto *media_card = new QFrame(this);
    media_card->setObjectName("mediaCard");
    auto *media_layout = new QVBoxLayout(media_card);
    auto *media_title = new QLabel("实时交互", media_card);
    media_title->setStyleSheet("font-size: 15pt; font-weight: 700; color: #162b4d;");
    media_layout->addWidget(media_title);
    m_media_preview_ = new QLabel("摄像头预览\n\n视频面试将在此显示", media_card);
    m_media_preview_->setAlignment(Qt::AlignCenter);
    m_media_preview_->setMinimumSize(260, 170);
    m_media_preview_->setStyleSheet(
        "background:#12233d; color:#a9c2e8; border-radius:8px; font-size:12pt;");
    media_layout->addWidget(m_media_preview_);
    media_layout->addWidget(new QLabel("交互模式", media_card));
    m_interaction_mode_ = new QComboBox(media_card);
    m_interaction_mode_->addItems({"文本对话", "语音对话（规划中）", "视频面试（规划中）"});
    m_interaction_mode_->setEnabled(false);
    media_layout->addWidget(m_interaction_mode_);
    auto *media_hint = new QLabel(
        "音频通过双向流式 RPC 传输，视频仅上传摄像头帧；服务端负责转写、打分和事件推送。",
        media_card);
    media_hint->setWordWrap(true);
    media_hint->setStyleSheet("color:#71809a; font-size:10pt;");
    media_layout->addWidget(media_hint);
    media_layout->addStretch();
    body_layout->addWidget(media_card, 3);
    layout->addLayout(body_layout, 1);
    // 进度条
    m_progress_bar_ = new QProgressBar(this);
    m_progress_bar_->setRange(0, 10);
    m_progress_bar_->setValue(0);
    m_progress_bar_->setTextVisible(true);
    m_progress_bar_->setFormat("进度: %v/%m 题");
    m_progress_bar_->setMinimumHeight(28);
    m_progress_bar_->setStyleSheet("QProgressBar {"
                                   "  border: 1px solid #ddd;"
                                   "  background-color: #f0f0f0;"
                                   "  text-align: center;"
                                   "  font-weight: bold;"
                                   "  font-size: 10pt;"
                                   "  color: #333;"
                                   "}"
                                   "QProgressBar::chunk {"
                                   "  background-color: #4CAF50;"
                                   "}");
    layout->addWidget(m_progress_bar_);

    QHBoxLayout *send_layout = new QHBoxLayout();
    m_btn_reply_msg_ = new QPushButton("提交回答", this);
    m_reply_msg_ = new QLineEdit(this);
    send_layout->addWidget(m_reply_msg_);
    send_layout->addWidget(m_btn_reply_msg_);
    layout->addLayout(send_layout);
    connect(m_btn_reply_msg_, &QPushButton::clicked, this, &MainWindow::OnReplyMessage);
    connect(m_reply_msg_, &QLineEdit::returnPressed, this, &MainWindow::OnReplyMessage);
    // 状态栏--左下角小字
    statusBar()->showMessage("就绪");
    setStyleSheet("QFrame#conversationCard, QFrame#mediaCard { background:#ffffff; border:1px "
                  "solid #dce5f2; border-radius:10px; } QPushButton { padding:7px 14px; "
                  "border-radius:6px; } QPushButton:hover { background:#e8f1ff; }");
}

} // namespace interview::ui
