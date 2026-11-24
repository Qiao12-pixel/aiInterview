#include "ui/MainWindow.h"
#include "ui/config_dialog.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTimer>
namespace interview::ui {
void MainWindow::OnViewReport() {
    if (busy_ || !completed_)
        return;
    if (worker_.joinable())
        worker_.join();
    SetBusy(true);
    worker_ = std::thread([this, id = session_id_] {
        try {
            const auto report = rpc_->Report(id);
            QMetaObject::invokeMethod(
                this,
                [this, report] {
                    const auto pretty = QJsonDocument::fromJson(QByteArray::fromStdString(report))
                                            .toJson(QJsonDocument::Indented);
                    AppendMessage(QString::fromUtf8(pretty), "#246947");
                    SetBusy(false);
                    m_status_area_->setText("● 报告已加载");
                },
                Qt::QueuedConnection);
        } catch (const std::exception &e) {
            const auto error = QString::fromUtf8(e.what());
            QMetaObject::invokeMethod(
                this,
                [this, error] {
                    AppendMessage(error, "#b54735");
                    SetBusy(false);
                },
                Qt::QueuedConnection);
        }
    });
}

} // namespace interview::ui
