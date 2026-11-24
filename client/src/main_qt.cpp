#include <QApplication>
#include <QCommandLineParser>
#include <QFile>
#include <QMessageBox>
#include "ui/MainWindow.h"
int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    QApplication::setApplicationName("AI Interview");
    QCommandLineParser parser;
    parser.addHelpOption();
    parser.addOption({"server", "gRPC 服务端 host:port", "address",
                      qEnvironmentVariable("AIINTERVIEW_SERVER", "127.0.0.1:50051")});
    parser.addOption({"tls", "使用 TLS 连接服务端"});
    parser.addOption({"ca", "可选 CA PEM 文件；默认使用系统信任证书", "file"});
    parser.process(app);
    QString roots;
    if (parser.isSet("ca")) {
        if (!parser.isSet("tls")) {
            QMessageBox::critical(nullptr, "配置错误", "--ca 需要与 --tls 同时使用");
            return 1;
        }
        QFile file(parser.value("ca"));
        if (!file.open(QIODevice::ReadOnly)) {
            QMessageBox::critical(nullptr, "配置错误", "无法读取 CA 文件");
            return 1;
        }
        roots = QString::fromUtf8(file.readAll());
    }
    interview::ui::MainWindow window(parser.value("server"), parser.isSet("tls"), roots);
    window.show();
    return app.exec();
}
