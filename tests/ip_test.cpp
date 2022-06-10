#include "ip.h"

#include <gtest/gtest.h>

using namespace ws;


TEST(IPAddrTest, MaximumLength) {
    EXPECT_EQ(IPv4Addr::max_length,
              std::string_view {"255.255.255.255"}.length());

    EXPECT_EQ(IPv6Addr::max_length,
              std::string_view {"FFFF:FFFF:FFFF:FFFF:FFFF:FFFF:255.255.255.255"}
                  .length());
}

TEST(IPAddrTest, LoopBack) {
    EXPECT_EQ(IPv4Addr::loop_back, "127.0.0.1");
    EXPECT_EQ(IPv6Addr::loop_back, "::1");
}

TEST(IPAddrTest, Any) {
    EXPECT_EQ(IPv4Addr::any, "0.0.0.0");
    EXPECT_EQ(IPv6Addr::any, "::");
}

TEST(IPAddrTest, Construction) {
    const std::string ipv4_loop {IPv4Addr::loop_back};
    const std::string ipv6_loop {IPv6Addr::loop_back};
    constexpr std::uint16_t port {1000};

    // Created from an IP address string and a port.
    const IPv4Addr ipv4_by_str {ipv4_loop, port};
    EXPECT_EQ(ipv4_by_str.IPAddress(), ipv4_loop);
    EXPECT_EQ(ipv4_by_str.Port(), port);

    const IPv6Addr ipv6_by_str {ipv6_loop, port};
    EXPECT_EQ(ipv6_by_str.IPAddress(), ipv6_loop);
    EXPECT_EQ(ipv6_by_str.Port(), port);

    // Created from a sock address structure.
    const IPv4Addr ipv4_by_addr {
        *reinterpret_cast<const sockaddr_in*>(ipv4_by_str.Raw())};
    EXPECT_EQ(ipv4_by_addr.IPAddress(), ipv4_loop);
    EXPECT_EQ(ipv4_by_addr.Port(), port);

    const IPv6Addr ipv6_by_addr {
        *reinterpret_cast<const sockaddr_in6*>(ipv6_by_str.Raw())};
    EXPECT_EQ(ipv6_by_addr.IPAddress(), ipv6_loop);
    EXPECT_EQ(ipv6_by_addr.Port(), port);
}

TEST(ConceptTest, ValidIPAddr) {
    EXPECT_TRUE(ValidIPAddr<IPv4Addr>);
    EXPECT_TRUE(ValidIPAddr<IPv6Addr>);

    EXPECT_FALSE(ValidIPAddr<IPAddr>);
    EXPECT_FALSE(ValidIPAddr<int>);
}