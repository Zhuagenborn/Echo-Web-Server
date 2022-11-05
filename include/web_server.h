/**
 * @file web_server.h
 * @brief The echo HTTP server.
 *
 * @author Zhenshuo Chen (chenzs108@outlook.com)
 * @author Liu Guowen (liu.guowen@outlook.com)
 * @par GitHub
 * https://github.com/Zhuagenborn
 * @version 1.0
 * @date 2022-06-30
 */

#pragma once

#include "containers/epoller.h"
#include "containers/heap_timer.h"
#include "containers/thread_pool.h"
#include "http.h"
#include "ip.h"
#include "log.h"
#include "util.h"

#include <sys/socket.h>

#include <atomic>
#include <cassert>
#include <filesystem>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <unordered_map>


namespace ws {

template <ValidIPAddr IPAddr>
class WebServer {
public:
    using Clock = std::chrono::steady_clock;

    using Ptr = std::shared_ptr<WebServer>;

    //! Set the root directory.
    static void SetRootDirectory(std::filesystem::path dir) noexcept {
        http::Connection<IPAddr>::SetRootDirectory(std::move(dir));
    }

    //! Get the root directory.
    static std::filesystem::path GetRootDirectory() noexcept {
        return http::Connection<IPAddr>::GetRootDirectory();
    }

    /**
     * @brief Create a web server.
     *
     * @param port          A listening port.
     * @param alive_time
     * A maximum alive time for client timers.
     * A client's timer will be refreshed if it sends or receives data.
     * When a client's timer reaches zero, it will disconnect.
     * @param logger
     * A logger. If it is @p nullptr, the thread pool will use the global root logger.
     */
    explicit WebServer(const std::uint16_t port,
                       const Clock::duration alive_time,
                       log::Logger::Ptr logger = log::RootLogger()) noexcept :
        port_ {port}, alive_time_ {alive_time}, logger_ {std::move(logger)} {
        if (!logger_) {
            logger_ = log::RootLogger();
        }
    }

    ~WebServer() noexcept {
        Close();
    }

    WebServer(const WebServer&) = delete;

    WebServer(WebServer&&) = delete;

    WebServer& operator=(const WebServer&) = delete;

    WebServer& operator=(WebServer&&) = delete;

    //! Start the server.
    void Start() {
        thread_pool_.Start();
        InitNetwork();
        while (!closed_) {
            try {
                const auto wait_time {timer_.ToNextTick()};
                const auto event_count {epoller_.Wait(wait_time)};
                for (auto i {0}; i != event_count; ++i) {
                    const auto socket {epoller_.FileDescriptor(i)};
                    const auto events {epoller_.Events(i)};
                    if (socket == listener_) {
                        OnListenEvent();
                    } else {
                        if (events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)) {
                            OnCloseEvent(socket);
                        } else if (events & EPOLLIN) {
                            OnReceiveEvent(socket);
                        } else if (events & EPOLLOUT) {
                            OnSendEvent(socket);
                        } else {
                            throw std::runtime_error {
                                fmt::format("Unexpected event: {}", events)};
                        }
                    }
                }
            } catch (const std::exception& err) {
                logger_->Log(log::Event::Create(log::Level::Error)
                             << fmt::format("Exception raised in server: {}",
                                            err.what()));
            }
        }
    }

    //! Close the server.
    void Close() noexcept {
        const std::lock_guard locker {mtx_};
        if (IsValidFileDescriptor(listener_)) {
            close(listener_);
            listener_ = invalid_file_descriptor;
        }

        thread_pool_.Close();
        epoller_.Close();
        timer_.Clear();
        users_.clear();
    }

private:
    static constexpr auto listen_event_mode {EPOLLRDHUP | EPOLLET};

    static constexpr auto connect_event_mode {EPOLLONESHOT | EPOLLRDHUP
                                              | EPOLLET};

    void InitNetwork() {
        assert(port_ >= 1024);

        const IPAddr addr {IPAddr::any.data(), port_};
        const linger opt {.l_onoff = true, .l_linger = 1};
        listener_ = socket(IPAddr::version, SOCK_STREAM, 0);
        if (!IsValidFileDescriptor(listener_)) {
            ThrowLastSystemError();
        }

        bool failed {true};
        const RAII raii {std::ref(listener_),
                         [&failed](FileDescriptor& socket) {
                             if (failed) {
                                 close(socket);
                                 socket = invalid_file_descriptor;
                             }
                         }};

        if (setsockopt(listener_, SOL_SOCKET, SO_LINGER, &opt, sizeof(opt))
            < 0) {
            ThrowLastSystemError();
        }

        const int enable {1};
        if (setsockopt(listener_, SOL_SOCKET, SO_REUSEADDR, &enable,
                       sizeof(enable))
            < 0) {
            ThrowLastSystemError();
        }

        if (bind(listener_, addr.Raw(), addr.Size()) < 0) {
            ThrowLastSystemError();
        }

        if (listen(listener_, SOMAXCONN) < 0) {
            ThrowLastSystemError();
        }

        SetFileDescriptorAsNonblocking(listener_);
        epoller_.AddFileDescriptor(listener_, listen_event_mode | EPOLLIN);
        failed = false;
    }

    //! A listen event is triggered.
    void OnListenEvent() {
        try {
            while (true) {
                typename IPAddr::RawType addr {};
                socklen_t size {sizeof(addr)};
                if (const auto new_socket {accept(
                        listener_, reinterpret_cast<sockaddr*>(&addr), &size)};
                    IsValidFileDescriptor(new_socket)) {
                    AddClient(new_socket, std::move(addr));
                } else {
                    ThrowLastSystemError();
                }
            }
        } catch (const std::system_error& err) {
            if (err.code() != std::errc::resource_unavailable_try_again) {
                logger_->Log(log::Event::Create(log::Level::Error)
                             << fmt::format("Failed to accept a new client: {}",
                                            err.what()));
                throw;
            }
        }
    }

    //! A close event is triggered.
    void OnCloseEvent(const FileDescriptor socket) noexcept {
        MarkClientAsToBeClosed(socket);
    }

    //! A receive event is triggered.
    void OnReceiveEvent(const FileDescriptor socket) {
        if (ExtendClientAliveTime(socket)) {
            auto client {Conn(socket)};
            thread_pool_.Push([client = std::move(client), this]() mutable {
                ReceiveFrom(std::move(client));
            });
        }
    }

    //! A send event is triggered.
    void OnSendEvent(const FileDescriptor socket) {
        if (ExtendClientAliveTime(socket)) {
            auto client {Conn(socket)};
            thread_pool_.Push([client = std::move(client), this]() mutable {
                SendTo(std::move(client));
            });
        }
    }

    /**
     * @brief Extend a socket's alive time.
     *
     * @return @p true if the corresponding client is still valid, otherwise @p false.
     */
    bool ExtendClientAliveTime(const FileDescriptor socket) {
        assert(IsValidFileDescriptor(socket));
        const std::lock_guard locker {mtx_};
        if (timer_.Contain(socket)) {
            timer_.Adjust(socket, alive_time_);
            return true;
        } else {
            return false;
        }
    }

    /**
     * @brief Mark a client as needing to be closed.
     *
     * @note
     * Clients should only be added and removed in the main thread.
     * This method makes a client's alive time zero,
     * so it will be removed in the next event loop of the main thread.
     */
    void MarkClientAsToBeClosed(const FileDescriptor socket) noexcept {
        assert(IsValidFileDescriptor(socket));
        const std::lock_guard locker {mtx_};
        assert(timer_.Contain(socket));
        timer_.Adjust(socket, Clock::duration::zero());
    }

    //! Add a client.
    void AddClient(const FileDescriptor socket, typename IPAddr::RawType addr) {
        assert(IsValidFileDescriptor(socket));

        const std::lock_guard locker {mtx_};
        SetFileDescriptorAsNonblocking(socket);
        epoller_.AddFileDescriptor(socket, connect_event_mode | EPOLLIN);

        IPAddr ip_addr {std::move(addr)};
        const auto ip_addr_str {ip_addr.IPAddress()};
        auto client {std::make_shared<http::Connection<IPAddr>>(
            socket, std::move(ip_addr))};
        users_.insert({socket, std::move(client)});

        timer_.Push(socket, alive_time_, [this](const auto socket) {
            logger_->Log(log::Event::Create(log::Level::Info)
                         << fmt::format("Client {} has timed-out",
                                        Conn(socket)->IPAddress()));
            CloseClient(socket);
        });

        logger_->Log(log::Event::Create(log::Level::Info) << fmt::format(
                         "A new client {} has connected", ip_addr_str));
        logger_->Log(log::Event::Create(log::Level::Debug)
                     << fmt::format("Client {} is bound to socket {}",
                                    ip_addr_str, socket));
    }

    //! Close a client.
    void CloseClient(const FileDescriptor socket) noexcept {
        assert(IsValidFileDescriptor(socket));

        const std::lock_guard locker {mtx_};
        const auto ip_addr {Conn(socket)->IPAddress()};

        try {
            epoller_.DeleteFileDescriptor(socket);
        } catch (const std::exception& err) {
            logger_->Log(log::Event::Create(log::Level::Debug) << fmt::format(
                             "Failed to delete socket {} from epoller: {}",
                             socket, err.what()));
        }

        timer_.Remove(socket);
        users_.erase(socket);
        logger_->Log(log::Event::Create(log::Level::Info)
                     << fmt::format("Client {} has disconnected", ip_addr));
    }

    //! Receive data from a client.
    void ReceiveFrom(typename http::Connection<IPAddr>::Ptr client) noexcept {
        const auto ip_addr {client->IPAddress()};

        try {
            logger_->Log(log::Event::Create(log::Level::Info) << fmt::format(
                             "Start to receive data from client {}", ip_addr));
            client->Receive();
            Process(std::move(client));
        } catch (const std::exception& err) {
            logger_->Log(log::Event::Create(log::Level::Error) << fmt::format(
                             "Failed to receive data from client {}: {}",
                             ip_addr, err.what()));

            MarkClientAsToBeClosed(client->Socket());
        }
    }

    //! Send data to a client.
    void SendTo(typename http::Connection<IPAddr>::Ptr client) noexcept {
        const auto ip_addr {client->IPAddress()};

        try {
            logger_->Log(log::Event::Create(log::Level::Info) << fmt::format(
                             "Start to send data to client {}", ip_addr));
            client->Send();
            if (client->KeepAlive()) {
                // Continue to receive data if the client keeps alive.
                Process(std::move(client));
                return;
            }
        } catch (const std::exception& err) {
            logger_->Log(log::Event::Create(log::Level::Error)
                         << fmt::format("Failed to send data to client {}: {}",
                                        ip_addr, err.what()));
        }

        MarkClientAsToBeClosed(client->Socket());
    }

    //! Process a client.
    void Process(const typename http::Connection<IPAddr>::Ptr client) {
        if (client->Process()) {
            // The client is ready for sending, register a send event.
            epoller_.ModifyFileDescriptor(client->Socket(),
                                          connect_event_mode | EPOLLOUT);
        } else {
            // The client's reading buffer for request is still empty, register a receive event.
            epoller_.ModifyFileDescriptor(client->Socket(),
                                          connect_event_mode | EPOLLIN);
        }
    }

    //! Get the client by a socket.
    typename http::Connection<IPAddr>::Ptr Conn(
        const FileDescriptor socket) noexcept {
        assert(users_.contains(socket));
        return users_[socket];
    }

    std::mutex mtx_;

    std::uint16_t port_;
    Clock::duration alive_time_;
    std::atomic_bool closed_ {false};

    FileDescriptor listener_ {invalid_file_descriptor};
    ThreadPool thread_pool_;
    Epoller epoller_;
    HeapTimer<FileDescriptor> timer_;
    std::unordered_map<FileDescriptor, typename http::Connection<IPAddr>::Ptr>
        users_;

    log::Logger::Ptr logger_;
};

/**
 * The builder for web servers.
 */
template <ValidIPAddr IPAddr>
class WebServerBuilder {
public:
    using Clock = std::chrono::steady_clock;

    static void SetRootDirectory(std::filesystem::path dir) noexcept {
        WebServer<IPAddr>::SetRootDirectory(std::move(dir));
    }

    static std::filesystem::path GetRootDirectory() noexcept {
        return WebServer<IPAddr>::GetRootDirectory();
    }

    WebServerBuilder& SetPort(const std::uint16_t port) noexcept {
        port_ = port;
        return *this;
    }

    WebServerBuilder& SetAliveTime(const Clock::duration time) noexcept {
        alive_time_ = time;
        return *this;
    }

    WebServerBuilder& SetLogger(log::Logger::Ptr logger) noexcept {
        logger_ = std::move(logger);
        return *this;
    }

    //! Create a web server with the current settings.
    WebServer<IPAddr> Create() noexcept {
        return WebServer<IPAddr> {port_, alive_time_, logger_};
    }

private:
    std::uint16_t port_;

    Clock::duration alive_time_;

    log::Logger::Ptr logger_;
};

}  // namespace ws