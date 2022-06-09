#include "containers/thread_pool.h"
#include "test_util.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <chrono>

using namespace ws;
using namespace ws::test;


TEST(ThreadPoolTest, Execution) {
    using namespace std::chrono_literals;

    class Task {
    public:
        MOCK_METHOD(void, OnInvoke, (), (const, noexcept));
    };

    const Task task;
    ThreadPool pool {std::nullopt, TestLogger()};
    pool.Start();
    constexpr std::size_t task_num {5};
    EXPECT_CALL(task, OnInvoke()).Times(task_num);
    for (auto i {0}; i != task_num; ++i) {
        pool.Push([&task]() { task.OnInvoke(); });
    }

    // Wait a short time for the logger to log events.
    // It is normal that a thread pool may not have executed all tasks when it is closed.
    std::this_thread::sleep_for(0.01s);
}