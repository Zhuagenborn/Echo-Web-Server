/**
 * @file ip.h
 * @brief The IP address interface.
 *
 * @author Zhenshuo Chen (chenzs108@outlook.com)
 * @par GitHub
 * https://github.com/Zhuagenborn
 * @version 1.0
 * @date 2022-06-06
 */

#pragma once

#include <concepts>
#include <cstdint>
#include <string>
#include <string_view>

#include <netinet/in.h>


namespace ws {

//! The interface of IP address.
class IPAddr {
public:
    virtual ~IPAddr() noexcept = default;

    //! Get the IP version.
    virtual int Version() const noexcept = 0;

    //! Get the size of socket address.
    virtual std::size_t Size() const noexcept = 0;

    //! Get the socket address.
    virtual const sockaddr* Raw() const noexcept = 0;

    virtual std::uint16_t Port() const noexcept = 0;

    virtual std::string IPAddress() const noexcept = 0;
};

//! The IPv4 address.
class IPv4Addr : public IPAddr {
public:
    static constexpr int version {AF_INET};

    static constexpr std::string_view loop_back {"127.0.0.1"};

    static constexpr std::string_view any {"0.0.0.0"};

    static constexpr std::size_t max_length {15};

    using RawType = sockaddr_in;

    explicit IPv4Addr(sockaddr_in addr);

    explicit IPv4Addr(std::string ip, std::uint16_t port);

    int Version() const noexcept override;

    std::size_t Size() const noexcept override;

    const sockaddr* Raw() const noexcept override;

    std::uint16_t Port() const noexcept override;

    std::string IPAddress() const noexcept override;

private:
    std::string ip_;
    sockaddr_in raw_ {};
};

//! The IPv6 address.
class IPv6Addr : public IPAddr {
public:
    static constexpr int version {AF_INET6};

    static constexpr std::string_view loop_back {"::1"};

    static constexpr std::string_view any {"::"};

    static constexpr std::size_t max_length {45};

    using RawType = sockaddr_in6;

    explicit IPv6Addr(sockaddr_in6 addr);

    explicit IPv6Addr(std::string ip, std::uint16_t port);

    int Version() const noexcept override;

    std::size_t Size() const noexcept override;

    const sockaddr* Raw() const noexcept override;

    std::uint16_t Port() const noexcept override;

    std::string IPAddress() const noexcept override;

private:
    std::string ip_;
    sockaddr_in6 raw_ {};
};

template <typename T>
concept ValidIPAddr = std::same_as<T, IPv4Addr> || std::same_as<T, IPv6Addr>;

}  // namespace ws