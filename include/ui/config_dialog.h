//
// Created by Joe on 26-4-1.
//

#ifndef CONFIG_DIALOG_H
#define CONFIG_DIALOG_H
#include <qdialog.h>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QGroupBox>
#include <QFormLayout>
#include <QSpinBox>
#include "MainWindow.h"
#include <QCheckBox>
#include <QString>
namespace interview {
    namespace ui {
        class ConfigDialog : public QDialog {
            Q_OBJECT
        public:
            explicit ConfigDialog(QWidget* parent = nullptr);
            ~ConfigDialog();

            QString GetCandidateName() const;
            QString GetResumePath() const;
            int GetMinQuestions() const;
            bool IsUseResume() const;
        private slots:
            void GetPDFPath();
        private:
            void SetupUi();
            QLineEdit* m_name_edit_;

            QCheckBox* m_use_resume_check_;
            QLineEdit* m_resume_edit_;
            QPushButton* m_btn_browse_;
            QSpinBox* m_questions_spin_box_;

            QPushButton* m_btn_save_info_;
            QPushButton* m_btn_cancel_info_;
        };
    }
}
#endif //CONFIG_DIALOG_H
