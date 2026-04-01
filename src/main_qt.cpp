#include <QApplication>
#include <QWidget>

#include "../include/ui/MainWindow.h"
//
// Created by Joe on 26-4-1.
//
int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    QApplication::setApplicationName("C++ Interview System");
    QApplication::setApplicationVersion("1.0");
    QApplication::setOrganizationName("Interview");

    interview::ui::MainWindow window;
    window.show();
    return app.exec();
}