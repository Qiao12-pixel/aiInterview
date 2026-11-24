#include "interview.grpc.pb.h"
#include <grpcpp/grpcpp.h>
#include <iostream>
int main(int argc, char **argv) {
    auto stub = aiinterview::v1::InterviewService::NewStub(grpc::CreateChannel(
        argc > 1 ? argv[1] : "127.0.0.1:50051", grpc::InsecureChannelCredentials()));
    grpc::ClientContext create;
    create.set_deadline(std::chrono::system_clock::now() + std::chrono::seconds(150));
    aiinterview::v1::CreateSessionRequest req;
    req.set_candidate_name("RPC smoke");
    req.set_question_total(1);
    aiinterview::v1::CreateSessionResponse res;
    auto status = stub->CreateSession(&create, req, &res);
    if (!status.ok()) {
        std::cerr << status.error_message();
        return 1;
    }
    if (res.first_question().text().empty())
        return 2;
    grpc::ClientContext answer;
    answer.set_deadline(std::chrono::system_clock::now() + std::chrono::seconds(150));
    aiinterview::v1::TextMessage text;
    text.set_session_id(res.session().id());
    text.set_text("RAII 利用对象生命周期，在析构时释放资源，确保异常安全。");
    auto reader = stub->SendText(&answer, text);
    aiinterview::v1::InterviewEvent event;
    bool feedback = false, next = false;
    while (reader->Read(&event)) {
        feedback |= event.type() == aiinterview::v1::InterviewEvent::FEEDBACK;
        next |= event.type() == aiinterview::v1::InterviewEvent::SUMMARY ||
                event.type() == aiinterview::v1::InterviewEvent::FOLLOW_UP;
    }
    if (!reader->Finish().ok() || !feedback || !next)
        return 3;
    std::cout << "gRPC create/answer passed\n";
}
