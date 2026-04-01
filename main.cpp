#include <iostream>
#include <QApplication>
#include <QWidget>
#include <QPushButton>
#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>
#include <curl/curl.h>
#include <portaudio.h>
#include <podofo/podofo.h>
#include <boost/asio.hpp>

using namespace std;
using json = nlohmann::json;
namespace asio = boost::asio;

/*int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    QWidget w;
    w.setWindowTitle("环境全部正常 ✅");
    w.show();
    return app.exec();
}*/