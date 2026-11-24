#include "account/account_dialog.h"
#include "transport/interview_client.h"
#include <QApplication>
#include <QLineEdit>
#include <QPushButton>
#include <QTimer>
#include <iostream>
int main(int argc,char** argv){
 QApplication app(argc,argv);
 interview::account::AccountDialog dialog(qEnvironmentVariable("TEST_ACCOUNT_API"));
 QTimer::singleShot(0,[&]{
  for(auto* edit:dialog.findChildren<QLineEdit*>()){
   if(edit->echoMode()==QLineEdit::Password)edit->setText(qEnvironmentVariable("TEST_ACCOUNT_PASSWORD"));
   else if(edit->placeholderText()=="name@example.com")edit->setText(qEnvironmentVariable("TEST_ACCOUNT_EMAIL"));
  }
  for(auto* button:dialog.findChildren<QPushButton*>())if(button->text()=="登录")button->click();
 });
 QTimer::singleShot(18000,&dialog,&QDialog::reject);
 if(dialog.exec()!=QDialog::Accepted||dialog.accessToken().isEmpty())return 1;
 interview::transport::InterviewClient rpc(qEnvironmentVariable("TEST_GRPC_TARGET").toStdString(),false,"",dialog.accessToken().toStdString());
 const auto session=rpc.Create("Qt Account Test",1,"");const auto events=rpc.Answer(session.id,"RAII 管理资源");
 if(events.empty()||events.back().type!="SUMMARY")return 2;
 if(rpc.Report(session.id).empty())return 3;
 std::cout<<"Qt account login and authenticated gRPC report passed\n";
}
