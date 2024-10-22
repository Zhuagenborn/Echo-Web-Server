/**
 * @file epoller.h
 * @brief The I/O event notification facility.
 *
 * @author Zhenshuo Chen (chenzs108@outlook.com)
 * @author Liu Guowen (liu.guowen@outlook.com)
 * @par GitHub
 * https://github.com/Zhuagenborn
 * @version 1.0
 * @date 2022-06-05
 */

#pragma once

#include "util.h"

#include <sys/epoll.h>

#include <chrono>
#include <optional>
#include <vector>


namespace ws {

/**
 * @brief
 * The I/O event notification facility.
 *
 * @details
 * It monitors multiple file descriptors to see if I/O is possible on any of them.
 */
class Epoller {
public:
    using Clock = std::chrono::steady_clock;

    /**
     * @brief Create an epoller.
     *
     * @param capacity The maximum number of file descriptors to be monitored.
     *
     * @exception std::system_error Creation failed.
     */
    explicit Epoller(std::size_t capacity = 1024);

    ~Epoller() noexcept;

    Epoller(const Epoller&) = delete;

    Epoller(Epoller&&) = delete;

    Epoller& operator=(const Epoller&) = delete;

    Epoller& operator=(Epoller&&) = delete;

    //! Close the epoller.
    void Close() noexcept;

    //! Add a file descriptor to the epoller.
    void AddFileDescriptor(ws::FileDescriptor fd, std::uint32_t events);

    //! Remove a file descriptor from the epoller.
    void DeleteFileDescriptor(ws::FileDescriptor fd);

    //! Change the setting associated with a file descriptor in the epoller.
    void ModifyFileDescriptor(ws::FileDescriptor fd, std::uint32_t events);

    /**
     * @brief Wait for events.
     *
     * @param time_out The maximum time to wait.
     * @return
     * The number of file descriptors ready for the requested I/O,
     * or zero if getting a time-out.
     *
     * @exception std::system_error Failed to wait.
     */
    std::size_t Wait(Clock::duration time_out);

    /**
     * @brief Get a file descriptor's trigger events.
     *
     * @note This method should be called after @p Wait returns.
     */
    std::uint32_t Events(std::size_t idx) const noexcept;

    /**
     * @brief Get a file descriptor.
     *
     * @note This method should be called after @p Wait returns.
     */
    ws::FileDescriptor FileDescriptor(std::size_t idx) const noexcept;

private:
    enum class Control { Add, Delete, Modify };

    /**
     * @brief Perform control operations on a file descriptor.
     *
     * @param ctl Addition, deletion or modification.
     * @param fd A file descriptor.
     * @param events
     * The event associated with the file descriptor.
     * It can be @p std::nullopt if the operation is deletion.
     *
     * @exception std::system_error An error occurred.
     */
    void SetFileDescriptor(Control ctl, ws::FileDescriptor fd,
                           std::optional<std::uint32_t> events = std::nullopt);

    bool ValidIndex(std::size_t idx) const noexcept;

    ws::FileDescriptor epoll_fd_ {invalid_file_descriptor};
    std::vector<epoll_event> events_;
};

}  // namespace ws