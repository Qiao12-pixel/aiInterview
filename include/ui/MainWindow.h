//
// Created by Joe on 26-4-1.
//

#ifndef MAINWINDOW_H
#define MAINWINDOW_H
#include <QMainWindow>
#include <QMenuBar>
#include <QMessageBox>
#include <QToolBar>
#include <QPushButton>
#include <QVBoxLayout>
#include <QTextEdit>
#include <QLabel>
#include <QProgressBar>
#include <QStatusBar>
namespace interview {
    namespace ui {
        class MainWindow : public QMainWindow {
            Q_OBJECT
        public:
            explicit MainWindow(QWidget* parent = nullptr);
            ~MainWindow();

        private slots:
            void OnNewSession();

        private:
            void SetupUi();
            void SetupMenuBar();
            void SetupToolBar();
            void SetupCentralWidget();
            void AppendMessage(const QString& text, const QString& color = "black");

        private:
            QPushButton* m_btn_new_session_;
            QPushButton* m_btn_start_interview_;
            QPushButton* m_btn_stop_session_;
            QPushButton* m_btn_view_report_;

        private:
            QTextEdit* m_message_area_;
            QLabel* m_status_area_;

            QProgressBar* m_progress_bar_;

            QString candidate_name_;
            QString resume_path_;
            int min_question_;
        };
    }
}
#endif //MAINWINDOW_H
