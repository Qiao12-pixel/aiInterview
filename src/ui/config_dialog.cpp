//
// Created by Joe on 26-4-1.
//
#include "../../include/ui/config_dialog.h"

#include <QVBoxLayout>

namespace interview {
    namespace ui {
        ConfigDialog::ConfigDialog(QWidget *parent) : QDialog(parent){
            SetupUi();
            setWindowTitle("新建面试会话");
            resize(500, 300);
        }
        ConfigDialog::~ConfigDialog() {

        }
        void ConfigDialog::SetupUi() {
            QVBoxLayout* main_layout = new QVBoxLayout(this);

            //候选人信息
            QGroupBox* candidate_group = new QGroupBox("候选人信息", this);
            QFormLayout* candidate_layout = new QFormLayout(candidate_group);

            m_name_edit_ = new QLineEdit(this);
            m_name_edit_->setPlaceholderText("请输入候选人姓名");
            candidate_layout->addRow("姓名: ", m_name_edit_);

            main_layout->addWidget(candidate_group);
            //简历设置
            QGroupBox* resume_group = new QGroupBox("简历设置", this);
            QVBoxLayout* resume_layout = new QVBoxLayout(resume_group);

            m_use_resume_check_ = new QCheckBox("使用简历驱动面试", this);
            m_use_resume_check_->setChecked(false);
            resume_layout->addWidget(m_use_resume_check_);

            QHBoxLayout* resume_path_layout = new QHBoxLayout();
            m_resume_edit_ = new QLineEdit(this);
            m_resume_edit_->setPlaceholderText("选择PDF简历文件");
            m_resume_edit_->setEnabled(false);
            resume_path_layout->addWidget(m_resume_edit_);

            m_btn_browse_ = new QPushButton("浏览...", this);
            m_btn_browse_->setEnabled(true);
            resume_path_layout->addWidget(m_btn_browse_);
            resume_layout->addLayout(resume_path_layout);

            QLabel* hint = new QLabel("提示：如不提供简历，将生成默认C++技术问题", this);
            hint->setStyleSheet("QLabel { color: #666; font-style: italic; }");
            resume_layout->addWidget(hint);

            main_layout->addWidget(resume_group);
            //问题设置
            QGroupBox* question_group = new QGroupBox(this);
            QFormLayout* question_layout = new QFormLayout(question_group);

            m_questions_spin_box_ = new QSpinBox(this);
            m_questions_spin_box_->setRange(1, 50);
            m_questions_spin_box_->setValue(50);
            m_questions_spin_box_->setSuffix(" 个");
            question_layout->addRow("问题数量(1~50):", m_questions_spin_box_);

            main_layout->addWidget(question_group);

            //按钮
            QHBoxLayout* button_layout = new QHBoxLayout();
            button_layout->addStretch();

            m_btn_cancel_interview_ = new QPushButton("取消", this);
            m_btn_start_interview_ = new QPushButton("开始面试", this);
            m_btn_start_interview_->setDefault(true);

            button_layout->addWidget(m_btn_cancel_interview_);
            button_layout->addWidget(m_btn_start_interview_);

            main_layout->addLayout(button_layout);
        }
        QString ConfigDialog::GetCandidateName() const {
            return m_name_edit_->text().trimmed();
        }
        QString ConfigDialog::GetResumePath() const {
            if (m_use_resume_check_->isChecked()) {
                return m_resume_edit_->text().trimmed();
            }
            return QString();
        }
        int ConfigDialog::GetMinQuestions() const {
            return m_questions_spin_box_->value();
        }
        bool ConfigDialog::IsUseResume() const {
            return m_use_resume_check_->isChecked();
        }





    }
}
