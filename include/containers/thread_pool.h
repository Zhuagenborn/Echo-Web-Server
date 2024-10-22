/**
 * @file thread_pool.h
 * @brief The thread pool.
 *
 * @author Zhenshuo Chen (chenzs108@outlook.com)
 * @author Liu Guowen (liu.guowen@outlook.com)
 * @par GitHub
 * https://github.com/Zhuagenborn
 * @version 1.0
 * @date 2022-06-04
 *
 * @example tests/containers/thread_pool_test.cpp
 */

#pragma once

#include "log.h"

#include <condition_variable>
#include <functional>
#include <list>
#include <mutex>
#include <optional>
#include <thread>


namespace ws {

//! The thread pool.
class ThreadPool {
public:
    using Task = std::function<void()>;

    /**
     * @brief Create a thread pool.
     *
     * @param thread_count
     * The number of working threads.
     * If it is @p std::nullopt or zero, the thread pool will use the number of concurrent threads supported by hardware.
     * @param logger
     * A logger. If it is @p nullptr, the thread pool will use the global root logger.
     */
    explicit ThreadPool(std::optional<std::size_t> thread_count = std::nullopt,
                        log::Logger::Ptr logger = log::RootLogger()) noexcept;

    ~ThreadPool() noexcept;

    ThreadPool(const ThreadPool&) = delete;

    ThreadPool(ThreadPool&&) = delete;

    ThreadPool& operator=(const ThreadPool&) = delete;

    ThreadPool& operator=(ThreadPool&&) = delete;

    //! Run the thread pool.
    void Start() noexcept;

    //! Push a task into the thread pool.
    void Push(Task task) noexcept;

    /**
     * @brief Close the thread pool.
     *
     * @details
     * The remaining tasks will not be executed.
     */
    void Close() noexcept;

private:
    /**
     * @brief Continually pop and execute tasks.
     *
     * @warning
     * Any exceptions raised in callbacks will not be rethrown.
     * Exception messages will be recorded in the logger.
     */
    void ExecProc() noexcept;

    log::Logger::Ptr logger_;

    mutable std::mutex mtx_;
    std::atomic_bool closed_ {true};
    std::size_t thread_count_;
    std::condition_variable cond_;

    std::list<Task> tasks_;
    std::list<std::thread> threads_;
};

}  // namespace ws