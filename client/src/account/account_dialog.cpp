#include "account/account_dialog.h"
#include <QEventLoop>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPushButton>
#include <QTimer>
#include <QUrl>
#include <QVBoxLayout>
#include <nlohmann/json.hpp>
namespace interview::account {
AccountDialog::AccountDialog(const QString &apiBase, QWidget *parent)
    : QDialog(parent), api_base_(apiBase) {
    setWindowTitle("登录 AI Interview");
    setModal(true);
    setMinimumWidth(380);
    auto *layout = new QVBoxLayout(this);
    auto *form = new QFormLayout;
    email_ = new QLineEdit(this);
    email_->setPlaceholderText("name@example.com");
    email_->setMaxLength(254);
    name_ = new QLineEdit(this);
    name_->setPlaceholderText("面试记录中显示的名称");
    name_->setMaxLength(80);
    name_->hide();
    password_ = new QLineEdit(this);
    password_->setEchoMode(QLineEdit::Password);
    password_->setMaxLength(128);
    password_->setPlaceholderText("至少 10 位");
    error_ = new QLabel(this);
    error_->setWordWrap(true);
    error_->setStyleSheet("color:#b54735");
    form->addRow("邮箱", email_);
    form->addRow("昵称", name_);
    form->addRow("密码", password_);
    layout->addLayout(form);
    layout->addWidget(error_);
    submit_ = new QPushButton("登录", this);
    toggle_ = new QPushButton("创建新账号", this);
    layout->addWidget(submit_);
    layout->addWidget(toggle_);
    connect(submit_, &QPushButton::clicked, this, &AccountDialog::Submit);
    connect(toggle_, &QPushButton::clicked, this, &AccountDialog::ToggleMode);
}
QString AccountDialog::accessToken() const { return access_token_; }
void AccountDialog::ToggleMode() {
    register_mode_ = !register_mode_;
    name_->setVisible(register_mode_);
    submit_->setText(register_mode_ ? "创建账号" : "登录");
    toggle_->setText(register_mode_ ? "已有账号？返回登录" : "创建新账号");
    error_->clear();
}
void AccountDialog::Submit() {
    if (email_->text().trimmed().isEmpty() || password_->text().size() < 10 ||
        (register_mode_ && name_->text().trimmed().isEmpty())) {
        error_->setText("请填写邮箱、密码" +
                        (register_mode_ ? QStringLiteral("和昵称") : QString{}) +
                        "；密码至少 10 位");
        return;
    }
    nlohmann::json body = {{"email", email_->text().trimmed().toStdString()},
                           {"password", password_->text().toStdString()}};
    if (register_mode_)
        body["display_name"] = name_->text().trimmed().toStdString();
    QNetworkAccessManager manager;
    QNetworkRequest request(
        QUrl(api_base_ + "/api/account/native-" + (register_mode_ ? "register" : "login")));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    const QUrl origin(api_base_);
    request.setRawHeader("Origin", (origin.scheme() + "://" + origin.authority()).toUtf8());
    submit_->setEnabled(false);
    toggle_->setEnabled(false);
    auto *reply = manager.post(request, QByteArray::fromStdString(body.dump()));
    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    timer.start(15000);
    loop.exec();
    submit_->setEnabled(true);
    toggle_->setEnabled(true);
    if (!timer.isActive()) {
        reply->abort();
        error_->setText("连接账号服务超时");
        reply->deleteLater();
        return;
    }
    timer.stop();
    const auto payload = nlohmann::json::parse(reply->readAll().toStdString(), nullptr, false);
    const auto networkError = reply->error();
    reply->deleteLater();
    if (networkError != QNetworkReply::NoError || !payload.is_object() ||
        !payload.contains("access_token")) {
        error_->setText(payload.is_object() ? QString::fromStdString(payload.value(
                                                  "message", std::string("账号服务连接失败")))
                                            : QStringLiteral("账号服务连接失败"));
        return;
    }
    access_token_ = QString::fromStdString(payload["access_token"].get<std::string>());
    accept();
}
} // namespace interview::account
