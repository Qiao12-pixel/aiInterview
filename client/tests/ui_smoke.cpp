#include "ui/MainWindow.h"
#include "ui/config_dialog.h"
#include <QApplication>
#include <QTimer>
#include <QSpinBox>
#include <QTest>
#include <iostream>
#include <stdexcept>
static void Wait(const std::function<bool()>& predicate) {
    for(int i=0;i<100 && !predicate();++i) QTest::qWait(50);
    if(!predicate()) throw std::runtime_error("UI did not reach expected state");
}
int main(int argc,char** argv) {
    QApplication app(argc,argv);
    interview::ui::MainWindow window("127.0.0.1:50061");window.show();
    try {
        QTimer::singleShot(0,[] {
            auto* dialog=qobject_cast<interview::ui::ConfigDialog*>(QApplication::activeModalWidget());
            if(!dialog) return;
            for(auto* input:dialog->findChildren<QLineEdit*>())
                if(input->placeholderText()=="请输入候选人姓名")input->setText("UI smoke");
            dialog->findChild<QSpinBox*>()->setValue(1);
            for(auto* button:dialog->findChildren<QPushButton*>())
                if(button->text()=="保存并关闭")button->click();
        });
        QMetaObject::invokeMethod(&window,"OnNewSession",Qt::DirectConnection);
        QMetaObject::invokeMethod(&window,"OnStartSession",Qt::DirectConnection);
        auto* input=window.findChild<QLineEdit*>();auto* messages=window.findChild<QTextEdit*>();
        Wait([&]{return input->isEnabled() && messages->toPlainText().contains("测试题");});
        input->setText("RAII 通过对象析构管理资源");
        QMetaObject::invokeMethod(&window,"OnReplyMessage",Qt::DirectConnection);
        QPushButton* report=nullptr;
        for(auto* button:window.findChildren<QPushButton*>())if(button->text()=="查看报告")report=button;
        Wait([&]{return report && report->isEnabled();});report->click();
        Wait([&]{return report->isEnabled() && messages->toPlainText().contains("records");});
        if(input->isEnabled())return 2;
        std::cout<<"Qt UI create/answer/report passed\n";
    } catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
