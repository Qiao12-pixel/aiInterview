#pragma once
#include <QDialog>
#include <QString>
class QLineEdit;
class QLabel;
class QPushButton;
namespace interview::account {
class AccountDialog final : public QDialog {
    Q_OBJECT
  public:
    explicit AccountDialog(const QString &apiBase, QWidget *parent = nullptr);
    QString accessToken() const;
  private slots:
    void ToggleMode();
    void Submit();

  private:
    QString api_base_, access_token_;
    bool register_mode_ = false;
    QLineEdit *email_ = nullptr;
    QLineEdit *name_ = nullptr;
    QLineEdit *password_ = nullptr;
    QLabel *error_ = nullptr;
    QPushButton *submit_ = nullptr;
    QPushButton *toggle_ = nullptr;
};
} // namespace interview::account
