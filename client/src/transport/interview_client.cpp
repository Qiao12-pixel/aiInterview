#include "transport/interview_client.h"
#include "interview.grpc.pb.h"
#include <grpcpp/grpcpp.h>
#include <chrono>
#include <mutex>
#include <stdexcept>
namespace interview::transport {
namespace rpc = aiinterview::v1;
class InterviewClient::Impl {
  public:
    std::unique_ptr<rpc::InterviewService::Stub> stub;
    std::mutex mutex;
    std::shared_ptr<grpc::ClientContext> current;
    bool stopping = false;
    std::shared_ptr<grpc::ClientContext> Context() {
        auto ctx = std::make_shared<grpc::ClientContext>();
        ctx->set_deadline(std::chrono::system_clock::now() + std::chrono::seconds(150));
        std::lock_guard<std::mutex> lock(mutex);
        current = ctx;
        if (stopping)
            ctx->TryCancel();
        return ctx;
    }
    void Finish(const grpc::Status &status) {
        {
            std::lock_guard<std::mutex> lock(mutex);
            current.reset();
        }
        if (!status.ok())
            throw std::runtime_error("gRPC " + std::to_string(status.error_code()) + ": " +
                                     status.error_message());
    }
};
InterviewClient::InterviewClient(const std::string &target, bool tls, const std::string &roots)
    : impl_(std::make_unique<Impl>()) {
    grpc::SslCredentialsOptions options;
    options.pem_root_certs = roots;
    grpc::ChannelArguments args;
    args.SetMaxReceiveMessageSize(8 * 1024 * 1024);
    impl_->stub = rpc::InterviewService::NewStub(grpc::CreateCustomChannel(
        target, tls ? grpc::SslCredentials(options) : grpc::InsecureChannelCredentials(), args));
}
InterviewClient::~InterviewClient() = default;
CreatedSession InterviewClient::Create(const std::string &candidate, unsigned count,
                                       const std::string &pdf) {
    auto ctx = impl_->Context();
    rpc::CreateSessionRequest req;
    rpc::CreateSessionResponse res;
    req.set_candidate_name(candidate);
    req.set_question_total(count);
    req.set_resume_pdf(pdf);
    impl_->Finish(impl_->stub->CreateSession(ctx.get(), req, &res));
    return {res.session().id(), res.first_question().text(), res.session().question_total()};
}
std::vector<Event> InterviewClient::Answer(const std::string &id, const std::string &text) {
    auto ctx = impl_->Context();
    rpc::TextMessage req;
    req.set_session_id(id);
    req.set_text(text);
    auto reader = impl_->stub->SendText(ctx.get(), req);
    rpc::InterviewEvent event;
    std::vector<Event> result;
    while (reader->Read(&event))
        if (event.type() != rpc::InterviewEvent::DELTA)
            result.push_back(
                {rpc::InterviewEvent::Type_Name(event.type()), event.text(), event.json_payload()});
    impl_->Finish(reader->Finish());
    return result;
}
std::string InterviewClient::Report(const std::string &id) {
    auto ctx = impl_->Context();
    rpc::ReportRequest req;
    rpc::ReportResponse res;
    req.set_session_id(id);
    impl_->Finish(impl_->stub->GetReport(ctx.get(), req, &res));
    return res.report_json();
}
void InterviewClient::Cancel() {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    impl_->stopping = true;
    if (impl_->current)
        impl_->current->TryCancel();
}
} // namespace interview::transport
