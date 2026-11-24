//
// Created by Joe on 26-4-1.
//
#include "../../include/ui/config_dialog.h"

#include <QVBoxLayout>
#include <QFileDialog>
#include <QLabel>
#include <QMessageBox>
namespace interview {
namespace ui {
ConfigDialog::ConfigDialog(QWidget *parent) : QDialog(parent) {
    SetupUi();
    setWindowTitle("新建面试会话");
    resize(500, 300);
}
ConfigDialog::~ConfigDialog() {}
void ConfigDialog::GetPDFPath() {
    QString PDF_path_ = QFileDialog::getOpenFileName(this, "选择 PDF 简历", {}, "PDF 文件 (*.pdf)");
    if (PDF_path_.isEmpty()) {
        return;
    }
    m_resume_edit_->setText(PDF_path_);
}

QString ConfigDialog::GetCandidateName() const { return m_name_edit_->text().trimmed(); }
QString ConfigDialog::GetResumePath() const {
    if (m_use_resume_check_->isChecked()) {
        return m_resume_edit_->text().trimmed();
    }
    return QString();
}
int ConfigDialog::GetMinQuestions() const { return m_questions_spin_box_->value(); }
bool ConfigDialog::IsUseResume() const { return m_use_resume_check_->isChecked(); }

} // namespace ui
} // namespace interview
