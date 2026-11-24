#include "transport/interview_client.h"
#include <fstream>
#include <iterator>
#include <iostream>
#include <stdexcept>
int main(int argc,char** argv) {
    try {
        interview::transport::InterviewClient client(argc>1?argv[1]:"127.0.0.1:50061",false);
        try {client.Create("invalid PDF",1,"not a PDF");return 4;}
        catch(const std::runtime_error& e) {if(std::string(e.what()).find("gRPC 3:")!=0) throw;}
        std::string pdf;
        if(argc>2) {std::ifstream in(argv[2],std::ios::binary);pdf.assign(std::istreambuf_iterator<char>(in),{});}
        const auto session=client.Create("Qt transport smoke",1,pdf);
        if(session.id.empty()||session.question.empty())return 1;
        const auto events=client.Answer(session.id,"RAII 依赖对象生命周期释放资源");
        if(events.size()!=2||events.back().type!="SUMMARY")return 2;
        if(client.Report(session.id).find("records")==std::string::npos)return 3;
        std::cout<<"Qt transport create/answer/report passed"<<(pdf.empty()?"":" with PDF upload")<<'\n';
    }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
