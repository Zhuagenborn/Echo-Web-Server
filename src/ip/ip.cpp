
#include "ip.h"
#include "util.h"

#include <arpa/inet.h>

#include <array>


namespace ws {

IPv4Addr::IPv4Addr(sockaddr_in addr) : raw_ {std::move(addr)} {
    std::array<char, max_length + 1> ip;
    if (inet_ntop(version, &raw_.sin_addr, ip.data(), ip.size())) {
        ip_ = ip.data();
    } else {
        ThrowLastSystemError();
    }
}

IPv4Addr::IPv4Addr(std::string ip, const std::uint16_t port) :
    ip_ {std::move(ip)} {
    raw_.sin_family = version;
    raw_.sin_port = htons(port);
    if (inet_pton(version, ip_.data(), &raw_.sin_addr) != 1) {
        ThrowLastSystemError();
    }
}

int IPv4Addr::Version() const noexcept {
    return version;
}

std::size_t IPv4Addr::Size() const noexcept {
    return sizeof(raw_);
}

std::uint16_t IPv4Addr::Port() const noexcept {
    return ntohs(raw_.sin_port);
}

std::string IPv4Addr::IPAddress() const noexcept {
    return ip_;
}

const sockaddr* IPv4Addr::Raw() const noexcept {
    return reinterpret_cast<const sockaddr*>(&raw_);
}

IPv6Addr::IPv6Addr(sockaddr_in6 addr) : raw_ {std::move(addr)} {
    std::array<char, max_length + 1> ip;
    if (inet_ntop(version, &raw_.sin6_addr, ip.data(), ip.size())) {
        ip_ = ip.data();
    } else {
        ThrowLastSystemError();
    }
}

IPv6Addr::IPv6Addr(std::string ip, const std::uint16_t port) :
    ip_ {std::move(ip)} {
    raw_.sin6_family = version;
    raw_.sin6_port = htons(port);
    if (inet_pton(version, ip_.data(), &raw_.sin6_addr) != 1) {
        ThrowLastSystemError();
    }
}

int IPv6Addr::Version() const noexcept {
    return version;
}

std::size_t IPv6Addr::Size() const noexcept {
    return sizeof(raw_);
}

std::uint16_t IPv6Addr::Port() const noexcept {
    return ntohs(raw_.sin6_port);
}

std::string IPv6Addr::IPAddress() const noexcept {
    return ip_;
}

const sockaddr* IPv6Addr::Raw() const noexcept {
    return reinterpret_cast<const sockaddr*>(&raw_);
}

}  // namespace ws