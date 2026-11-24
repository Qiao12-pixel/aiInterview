#include "infrastructure/storage/connection_pool.h"
#include <atomic>
#include <future>
#include <cassert>
struct Connection {
    bool healthy = true;
};
int main() {
    std::atomic<int> created{0}, destroyed{0};
    auto pool = std::make_shared<backend::storage::Pool<Connection>>(
        1,
        [&] {
            ++created;
            return new Connection;
        },
        [&](auto *c) {
            ++destroyed;
            delete c;
        },
        [](auto *c) { return c->healthy; });
    std::weak_ptr<backend::storage::Pool<Connection>> weak = pool;
    {
        auto first = pool->Borrow();
        std::promise<void> entered;
        auto waiting = std::async(std::launch::async, [&] {
            entered.set_value();
            return pool->Borrow();
        });
        entered.get_future().wait();
        assert(waiting.wait_for(std::chrono::milliseconds(30)) == std::future_status::timeout);
        assert(created == 1);
        first.reset();
        auto second = waiting.get();
        assert(created == 1);
        second->healthy = false;
        second.reset();
        assert(destroyed == 1);
        auto replacement = pool->Borrow();
        assert(created == 2);
        pool.reset();
        assert(!weak.expired());
        assert(destroyed == 1);
    }
    assert(weak.expired());
    assert(destroyed == 2);
    bool fail = true;
    auto recovery = std::make_shared<backend::storage::Pool<Connection>>(
        1,
        [&]() -> Connection * {
            if (fail)
                throw std::runtime_error("offline");
            return new Connection;
        },
        [](auto *c) { delete c; }, [](auto *) { return true; });
    try {
        auto connection = recovery->Borrow();
        return 2;
    } catch (const std::runtime_error &) {
    }
    fail = false;
    auto recovered = recovery->Borrow();
    assert(recovered);
}
