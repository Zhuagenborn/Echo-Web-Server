#include "epoller.h"

#include <cassert>
#include <limits>
#include <unordered_map>


namespace ws {

Epoller::Epoller(const std::size_t capacity) : epoll_fd_ {epoll_create1(0)} {
    assert(capacity > 0);
    events_.resize(capacity);
    if (!IsValidFileDescriptor(epoll_fd_)) {
        ThrowLastSystemError();
    }
}

Epoller::~Epoller() noexcept {
    Close();
}

std::uint32_t Epoller::Events(const std::size_t idx) const noexcept {
    assert(ValidIndex(idx));
    return events_[idx].events;
}

FileDescriptor Epoller::FileDescriptor(const std::size_t idx) const noexcept {
    assert(ValidIndex(idx));
    return events_[idx].data.fd;
}

bool Epoller::ValidIndex(const std::size_t idx) const noexcept {
    return idx < events_.size();
}

void Epoller::Close() noexcept {
    if (IsValidFileDescriptor(epoll_fd_)) {
        close(epoll_fd_);
        epoll_fd_ = invalid_file_descriptor;
    }
}

void Epoller::AddFileDescriptor(const ws::FileDescriptor fd,
                                const std::uint32_t events) {
    SetFileDescriptor(Control::Add, fd, events);
}

void Epoller::DeleteFileDescriptor(const ws::FileDescriptor fd) {
    SetFileDescriptor(Control::Delete, fd);
}

void Epoller::ModifyFileDescriptor(const ws::FileDescriptor fd,
                                   const std::uint32_t events) {
    SetFileDescriptor(Control::Modify, fd, events);
}

void Epoller::SetFileDescriptor(const Control ctl, const ws::FileDescriptor fd,
                                const std::optional<std::uint32_t> events) {
    static const std::unordered_map<Control, int> op_codes {
        {Control::Add, EPOLL_CTL_ADD},
        {Control::Delete, EPOLL_CTL_DEL},
        {Control::Modify, EPOLL_CTL_MOD},
    };

    assert(IsValidFileDescriptor(fd));
    assert(op_codes.contains(ctl));

    epoll_event event {};
    event.data.fd = fd;
    if (ctl != Control::Delete) {
        assert(events.has_value());
        event.events = events.value();
    }

    if (epoll_ctl(epoll_fd_, op_codes.at(ctl), fd, &event) < 0) {
        ThrowLastSystemError();
    }
}

std::size_t Epoller::Wait(const Clock::duration time_out) {
    const auto size {events_.size()};
    assert(size <= std::numeric_limits<int>::max());

    const auto milliseconds {
        std::chrono::duration_cast<std::chrono::milliseconds>(time_out)
            .count()};
    assert(milliseconds <= std::numeric_limits<int>::max());

    if (const auto ready_count {
            epoll_wait(epoll_fd_, events_.data(), size, milliseconds)};
        ready_count >= 0) {
        return ready_count;
    } else if (static_cast<std::errc>(errno) == std::errc::interrupted) {
        // Some signal handlers will interrupt `epoll_wait` and similar system calls on any Linux.
        // This is by design.
        // One typical solution is to explicitly check `errno` for `EINTR`.
        return 0;
    } else {
        ThrowLastSystemError();
    }
}

}  // namespace ws