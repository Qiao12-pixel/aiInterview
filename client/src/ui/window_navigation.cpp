#include "ui/MainWindow.h"
#include "ui/config_dialog.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTimer>
namespace interview::ui {
void MainWindow::SetupUi() {
    setWindowTitle("AI Interview · 智能面试工作台");
    resize(1180, 760);
    setMinimumSize(980, 640);

    QWidget *central = new QWidget(this);
    setCentralWidget(central);
}
void MainWindow::SetupMenuBar() {
    // QMenuBar* menu_bar = new QMenuBar(this);
    // setMenuBar(menu_bar);
    QMenuBar *menu_bar = menuBar();
    menu_bar->setVisible(true);

    // 文件菜单
    QMenu *file_menu = menu_bar->addMenu("文件(&F)");
    QAction *new_action = file_menu->addAction("新建会话(&N)");
    new_action->setShortcut(QKeySequence::New);
    connect(new_action, &QAction::triggered, this, &MainWindow::OnNewSession);

    file_menu->addSeparator();

    QAction *exit_action = file_menu->addAction("退出(&X)");
    exit_action->setShortcut(QKeySequence::Quit);
    connect(exit_action, &QAction::triggered, this, &QWidget::close);

    QMenu *help_menu = menu_bar->addMenu("帮助(&H)");
    QAction *about_action = help_menu->addAction("关于(&A)");
    connect(about_action, &QAction::triggered, this, [this]() {
        QMessageBox::about(this, "关于",
                           "C++智能面试系统 v1.0\n"
                           "基于 C++ gRPC 服务的技术面试平台\n"
                           "支持简历驱动问题生成与智能评分");
    });
}
void MainWindow::SetupToolBar() {
    QToolBar *tool_bar = addToolBar("主工具栏");
    tool_bar->setMovable(false);

    m_btn_new_session_ = new QPushButton("新建会话", this);
    m_btn_new_session_->setDefault(true);
    m_btn_start_interview_ = new QPushButton("开始面试", this);
    m_btn_stop_session_ = new QPushButton("停止面试", this);
    m_btn_view_report_ = new QPushButton("查看报告", this);
    tool_bar->addWidget(m_btn_new_session_);
    tool_bar->addSeparator();
    tool_bar->addWidget(m_btn_start_interview_);
    tool_bar->addSeparator();
    tool_bar->addWidget(m_btn_stop_session_);
    tool_bar->addSeparator();
    tool_bar->addWidget(m_btn_view_report_);
    tool_bar->addSeparator();

    connect(m_btn_new_session_, &QPushButton::clicked, this, &MainWindow::OnNewSession);
    connect(m_btn_start_interview_, &QPushButton::clicked, this, &MainWindow::OnStartSession);
    connect(m_btn_stop_session_, &QPushButton::clicked, this, &QWidget::close);
    connect(m_btn_view_report_, &QPushButton::clicked, this, &MainWindow::OnViewReport);
}

} // namespace interview::ui
