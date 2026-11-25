#pragma once
#include "api/service.h"
namespace backend::api {
// A missing optional means execute locally; a present Status is the final remote/error result.
std::optional<grpc::Status> RouteCreate(cluster::Router *, Context *,
                                        const aiinterview::v1::CreateSessionRequest *,
                                        aiinterview::v1::CreateSessionResponse *);
std::optional<grpc::Status> RouteStart(cluster::Router *, Context *,
                                       const aiinterview::v1::CreateSessionRequest *,
                                       Writer<aiinterview::v1::InterviewEvent> *);
std::optional<grpc::Status> RouteAnswer(cluster::Router *, Context *,
                                        const aiinterview::v1::TextMessage *,
                                        Writer<aiinterview::v1::InterviewEvent> *);
} // namespace backend::api
