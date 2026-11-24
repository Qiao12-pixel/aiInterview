#include "interview.grpc.pb.h"
#include <grpcpp/grpcpp.h>
#include <nlohmann/json.hpp>
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <mutex>
#include <thread>
using Clock=std::chrono::steady_clock;
struct Stats {std::mutex mutex;std::vector<double> latencies;std::map<int,int> errors;unsigned ok=0;
 void Add(double ms,const grpc::Status& status){std::lock_guard<std::mutex> lock(mutex);latencies.push_back(ms);if(status.ok())++ok;else ++errors[status.error_code()];}
 nlohmann::json Result(){std::sort(latencies.begin(),latencies.end());auto q=[&](double p){return latencies.empty()?0:latencies[std::min(latencies.size()-1,static_cast<size_t>(p*latencies.size()))];};return {{"success",ok},{"errors",errors},{"p50_ms",q(.5)},{"p95_ms",q(.95)},{"p99_ms",q(.99)},{"max_ms",q(1)}};}};
int main(int argc,char**argv){
 if(argc!=4)return 2;unsigned concurrency=std::stoul(argv[2]),count=std::stoul(argv[3]);const char* token=std::getenv("BENCH_AUTH_TOKEN");
 auto stub=aiinterview::v1::InterviewService::NewStub(grpc::CreateChannel(argv[1],grpc::InsecureChannelCredentials()));
 auto setup=[&](grpc::ClientContext& ctx){ctx.set_deadline(std::chrono::system_clock::now()+std::chrono::seconds(15));if(token)ctx.AddMetadata("authorization",std::string("Bearer ")+token);};
 Stats create,answer,report,cycle;std::atomic_uint next{0};std::vector<std::thread> threads;const auto start=Clock::now();
 for(unsigned t=0;t<concurrency;++t)threads.emplace_back([&]{while(next.fetch_add(1)<count){const auto begin=Clock::now();grpc::ClientContext c;setup(c);aiinterview::v1::CreateSessionRequest req;req.set_candidate_name("Load test synthetic");req.set_question_total(1);aiinterview::v1::CreateSessionResponse session;
 auto stage=Clock::now();auto status=stub->CreateSession(&c,req,&session);create.Add(std::chrono::duration<double,std::milli>(Clock::now()-stage).count(),status);
 if(status.ok()){grpc::ClientContext a;setup(a);aiinterview::v1::TextMessage text;text.set_session_id(session.session().id());text.set_text("RAII binds resource lifetime to object lifetime.");stage=Clock::now();auto reader=stub->SendText(&a,text);aiinterview::v1::InterviewEvent event;bool summary=false;while(reader->Read(&event))summary|=event.type()==aiinterview::v1::InterviewEvent::SUMMARY;status=reader->Finish();if(status.ok()&&!summary)status={grpc::StatusCode::DATA_LOSS,"summary missing"};answer.Add(std::chrono::duration<double,std::milli>(Clock::now()-stage).count(),status);}
 if(status.ok()){grpc::ClientContext r;setup(r);aiinterview::v1::ReportRequest request;request.set_session_id(session.session().id());aiinterview::v1::ReportResponse result;stage=Clock::now();status=stub->GetReport(&r,request,&result);report.Add(std::chrono::duration<double,std::milli>(Clock::now()-stage).count(),status);}
 cycle.Add(std::chrono::duration<double,std::milli>(Clock::now()-begin).count(),status);
 }});
 for(auto& thread:threads)thread.join();const auto seconds=std::chrono::duration<double>(Clock::now()-start).count();std::cout<<nlohmann::json{{"concurrency",concurrency},{"count",count},{"seconds",seconds},{"successful_cycles_per_second",cycle.ok/seconds},{"create",create.Result()},{"answer",answer.Result()},{"report",report.Result()},{"cycle",cycle.Result()}}.dump()<<std::endl;
}
