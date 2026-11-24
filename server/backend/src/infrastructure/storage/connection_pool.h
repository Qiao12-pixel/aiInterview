#pragma once
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <vector>
#include <chrono>
#include <stdexcept>
namespace backend::storage {
// 每个借用独占一个连接；归还前检查状态，不重放任何有副作用操作。
template <class T> class Pool : public std::enable_shared_from_this<Pool<T>> {
    std::function<T *()> create_;
    std::function<void(T *)> destroy_;
    std::function<bool(T *)> healthy_;
    size_t limit_, total_ = 0;
    std::mutex mutex_;
    std::condition_variable available_;
    std::vector<T *> idle_;

  public:
    using Lease = std::unique_ptr<T, std::function<void(T *)>>;
    Pool(size_t limit, std::function<T *()> create, std::function<void(T *)> destroy,
         std::function<bool(T *)> healthy)
        : create_(std::move(create)), destroy_(std::move(destroy)), healthy_(std::move(healthy)),
          limit_(limit) {
        if (!limit || limit > 64)
            throw std::runtime_error("连接池大小须为 1–64");
    }
    ~Pool() {
        for (auto *connection : idle_)
            destroy_(connection);
    }
    Lease Borrow() {
        T *connection = nullptr;
        {
            std::unique_lock<std::mutex> lock(mutex_);
            if (!available_.wait_for(lock, std::chrono::seconds(3),
                                     [&] { return !idle_.empty() || total_ < limit_; }))
                throw std::runtime_error("连接池繁忙，请稍后重试");
            if (!idle_.empty()) {
                connection = idle_.back();
                idle_.pop_back();
            } else
                ++total_;
        }
        try {
            if (connection && !healthy_(connection)) {
                destroy_(connection);
                connection = nullptr;
            }
            if (!connection)
                connection = create_();
            if (!connection)
                throw std::runtime_error("连接创建失败");
        } catch (...) {
            if (connection)
                destroy_(connection);
            std::lock_guard<std::mutex> lock(mutex_);
            --total_;
            available_.notify_one();
            throw;
        }
        return Lease(connection, [self = this->shared_from_this()](T *value) {
            bool healthy = false;
            try {
                healthy = self->healthy_(value);
            } catch (...) {
            }
            if (!healthy)
                self->destroy_(value);
            {
                std::lock_guard<std::mutex> lock(self->mutex_);
                if (healthy)
                    self->idle_.push_back(value);
                else
                    --self->total_;
            }
            self->available_.notify_one();
        });
    }
};
} // namespace backend::storage
