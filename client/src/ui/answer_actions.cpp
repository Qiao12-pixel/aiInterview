#include "ui/MainWindow.h"
#include "ui/config_dialog.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTimer>
namespace interview::ui {
void MainWindow::OnReplyMessage() {
    if (busy_ || session_id_.empty() || completed_)
        return;
    const auto answer = m_reply_msg_->text().trimmed();
    if (answer.isEmpty())
        return;
    if (worker_.joinable())
        worker_.join();
    AppendMessage(candidate_name_ + "：" + answer, "#92502a");
    m_reply_msg_->clear();
    SetBusy(true);
    worker_ = std::thread([this, id = session_id_, answer] {
        try {
            const auto events = rpc_->Answer(id, answer.toStdString());
            QMetaObject::invokeMethod(
                this,
                [this, events] {
                    for (const auto &event : events) {
                        const QString prefix = event.type == "FEEDBACK"  ? "评价："
                                               : event.type == "SUMMARY" ? "总结："
                                                                         : "面试官：";
                        AppendMessage(prefix + QString::fromStdString(event.text), "#0066cc");
                        if (event.type == "QUESTION") {
                            const auto payload =
                                QJsonDocument::fromJson(QByteArray::fromStdString(event.json))
                                    .object();
                            m_progress_bar_->setValue(
                                payload.value("question_index").toInt(m_progress_bar_->value()));
                        }
                        if (event.type == "SUMMARY")
                            completed_ = true;
                    }
                    m_status_area_->setText(completed_ ? "● 面试已完成，可查看报告"
                                                       : "● 请继续回答");
                    SetBusy(false);
                    if (!completed_)
                        m_reply_msg_->setFocus();
                },
                Qt::QueuedConnection);
        } catch (const std::exception &e) {
            const auto error = QString::fromUtf8(e.what());
            QMetaObject::invokeMethod(
                this,
                [this, error, answer] {
                    AppendMessage(error, "#b54735");
                    m_reply_msg_->setText(answer);
                    m_status_area_->setText("● 请求失败；连接中断时请勿直接重复提交，可新建会话");
                    SetBusy(false);
                },
                Qt::QueuedConnection);
        }
    });
}

} // namespace interview::ui
