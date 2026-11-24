#pragma once
#include "interview/ports/model.h"
namespace backend::llm {
class StreamDecoder {
  public:
    explicit StreamDecoder(TokenSink sink) : sink_(std::move(sink)) {}
    void Feed(const std::string &bytes);
    const std::string &Content() const { return content_; }
    bool Complete() const { return done_; }

  private:
    TokenSink sink_;
    std::string buffer_, content_;
    bool done_ = false;
};
} // namespace backend::llm
