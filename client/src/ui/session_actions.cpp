#include "ui/MainWindow.h"
#include "ui/config_dialog.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTimer>
namespace interview::ui {
void MainWindow::OnNewSession() {
    if (busy_)
        return;
    ConfigDialog dialog(this);
    if (dialog.exec() != QDialog::Accepted)
        return;
    candidate_name_ = dialog.GetCandidateName();
    resume_path_ = dialog.GetResumePath();
    min_question_ = dialog.GetMinQuestions();
    session_id_.clear();
    completed_ = false;
    m_message_area_->clear();
    m_reply_msg_->clear();
    m_progress_bar_->setRange(0, min_question_);
    m_progress_bar_->setValue(0);
    AppendMessage(QString("候选人：%1 · %2 题 · 服务端：%3")
                      .arg(candidate_name_)
                      .arg(min_question_)
                      .arg(endpoint_));
    if (!resume_path_.isEmpty())
        AppendMessage("简历将在开始面试时上传到服务端解析。");
    m_status_area_->setText("● 会话已配置，请开始面试");
    SetBusy(false);
}
void MainWindow::OnStartSession() {
    if (busy_ || !session_id_.empty())
        return;
    if (candidate_name_.trimmed().isEmpty()) {
        QMessageBox::warning(this, "提示", "请填写候选人姓名");
        return;
    }
    std::string pdf;
    if (!resume_path_.isEmpty()) {
        QFile file(resume_path_);
        if (!file.open(QIODevice::ReadOnly) || file.size() > 5 * 1024 * 1024) {
            QMessageBox::warning(this, "简历错误", "无法读取简历，或文件超过 5 MiB");
            return;
        }
        const auto bytes = file.readAll();
        pdf.assign(bytes.constData(), bytes.size());
        if (pdf.empty()) {
            QMessageBox::warning(this, "简历错误", "简历文件为空");
            return;
        }
    }
    if (worker_.joinable())
        worker_.join();
    SetBusy(true);
    m_status_area_->setText("● 服务端正在解析简历并生成问题");
    worker_ = std::thread([this, name = candidate_name_.toStdString(), count = min_question_,
                           pdf = std::move(pdf)] {
        try {
            const auto session = rpc_->Create(name, count, pdf);
            QMetaObject::invokeMethod(
                this,
                [this, session] {
                    session_id_ = session.id;
                    completed_ = false;
                    m_progress_bar_->setRange(0, session.total);
                    m_progress_bar_->setValue(1);
                    AppendMessage("面试官：" + QString::fromStdString(session.question), "#0066cc");
                    m_status_area_->setText("● 请在下方输入回答并提交");
                    SetBusy(false);
                    m_reply_msg_->setFocus();
                },
                Qt::QueuedConnection);
        } catch (const std::exception &e) {
            const auto error = QString::fromUtf8(e.what());
            QMetaObject::invokeMethod(
                this,
                [this, error] {
                    AppendMessage(error, "#b54735");
                    m_status_area_->setText("● 启动失败，请检查服务连接后重试");
                    SetBusy(false);
                },
                Qt::QueuedConnection);
        }
    });
}

} // namespace interview::ui
