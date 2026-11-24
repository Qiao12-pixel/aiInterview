#include "ui/MainWindow.h"
#include "ui/config_dialog.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTimer>
namespace interview::ui {
MainWindow::MainWindow(const QString &server, bool tls, const QString &roots,
                       const QString &authToken, QWidget *parent)
    : QMainWindow(parent),
      rpc_(std::make_unique<transport::InterviewClient>(
          server.toStdString(), tls, roots.toStdString(), authToken.toStdString())),
      endpoint_(server), candidate_name_("候选人"), min_question_(10) {
    setMenuBar(new QMenuBar(this));
    SetupUi();
    SetupMenuBar();
    SetupToolBar();
    SetupCentralWidget();
    SetBusy(false);
    auto *timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, [this, elapsed = 0]() mutable {
        if (busy_)
            m_status_area_->setText(QString("● 服务端处理中 · %1 秒").arg(++elapsed));
        else
            elapsed = 0;
    });
    timer->start(1000);
    statusBar()->showMessage("服务端：" + endpoint_);
}
MainWindow::~MainWindow() {
    rpc_->Cancel();
    if (worker_.joinable())
        worker_.join();
}

void MainWindow::SetBusy(bool busy) {
    busy_ = busy;
    m_btn_new_session_->setEnabled(!busy);
    m_btn_start_interview_->setEnabled(!busy && session_id_.empty());
    m_btn_view_report_->setEnabled(!busy && completed_);
    const bool can_answer = !busy && !session_id_.empty() && !completed_;
    m_reply_msg_->setEnabled(can_answer);
    m_btn_reply_msg_->setEnabled(can_answer);
    m_btn_reply_msg_->setText(busy ? "处理中…" : "提交回答");
}
void MainWindow::AppendMessage(const QString &text, const QString &color) {
    QString html_text = QString("<span style='color: %1;'>%2</span><br>")
                            .arg(color)
                            .arg(text.toHtmlEscaped().replace("\n", "<br>"));
    m_message_area_->moveCursor(QTextCursor::End);
    m_message_area_->insertHtml(html_text);
    m_message_area_->moveCursor(QTextCursor::End);
    m_message_area_->ensureCursorVisible();
}

} // namespace interview::ui
