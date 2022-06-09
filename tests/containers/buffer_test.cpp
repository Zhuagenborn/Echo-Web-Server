#include "containers/buffer.h"
#include "io.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <sstream>

using namespace ws;


TEST(BufferTest, Construction) {
    {
        // Default construction
        const Buffer buf;
        EXPECT_TRUE(buf.Empty());
        EXPECT_EQ(buf.ReadableSize(), 0);
        EXPECT_TRUE(buf.ReadableString().empty());
        EXPECT_EQ(buf.Peek(), std::nullopt);
    }

    {
        // Created from an empty string.
        const Buffer buf {""};
        EXPECT_TRUE(buf.Empty());
        EXPECT_EQ(buf.ReadableSize(), 0);
        EXPECT_TRUE(buf.ReadableString().empty());
        EXPECT_EQ(buf.Peek(), std::nullopt);
    }

    const std::string str {"hello"};

    {
        // Created from a string.
        const Buffer buf {str};
        EXPECT_FALSE(buf.Empty());
        EXPECT_EQ(buf.ReadableSize(), str.length());
        EXPECT_EQ(buf.ReadableString(), str);
        EXPECT_EQ(buf.Peek(), static_cast<std::byte>(str.front()));
    }

    // Convert "hello" to bytes.
    std::vector<std::byte> bytes;
    std::ranges::for_each(str, [&bytes](const auto c) noexcept {
        bytes.push_back(static_cast<std::byte>(c));
    });

    {
        // Created from bytes.
        const Buffer buf {bytes};
        EXPECT_FALSE(buf.Empty());
        EXPECT_EQ(buf.ReadableSize(), bytes.size());
        EXPECT_EQ(buf.Peek(), bytes.front());
        EXPECT_EQ(buf.ReadableString(), str);

        const auto readable_bytes {buf.ReadableBytes()};
        EXPECT_EQ((std::vector<std::byte> {readable_bytes.begin(),
                                           readable_bytes.end()}),
                  bytes);
    }
}

TEST(BufferTest, Copy) {
    const Buffer src {"hello"};
    const Buffer buf {src};
    EXPECT_EQ(buf.ReadableSize(), src.ReadableSize());
    EXPECT_EQ(buf.WritableSize(), src.WritableSize());
    EXPECT_EQ(buf.ReadableString(), src.ReadableString());
}

TEST(BufferTest, ReadWriteOffset) {
    constexpr size_t init_size {0x10};
    Buffer buf {init_size};
    EXPECT_EQ(buf.WritableSize(), init_size);

    // `""` → `"hello"`
    std::string str {"hello"};
    assert(str.length() >= 3);
    buf.Append(str);
    EXPECT_EQ(buf.ReadableSize(), str.length());

    // `"hello"` → `"ello"`
    str = str.substr(1);
    buf.Retrieve(1);
    EXPECT_EQ(buf.ReadableSize(), str.length());

    // `"ello"` → `"lo"`
    str = str.substr(2);
    EXPECT_EQ(buf.RetrieveUntil(buf.ReadableBytes().data() + 2), 2);
    EXPECT_EQ(buf.ReadableSize(), str.length());

    // `"lo"` → `""`
    const auto size {buf.ReadableSize()};
    EXPECT_EQ(buf.RetrieveAll(), size);
    EXPECT_TRUE(buf.Empty());

    buf.EnsureWriteableSize(0x1000);
    EXPECT_GE(buf.WritableSize(), 0x1000);
}

TEST(BufferTest, ReadWrite) {
    constexpr std::size_t init_size {0x10};
    Buffer buf {init_size};

    // `""` → `"1"`
    const std::string str1 {"1"};
    buf.Append(str1);
    EXPECT_EQ(buf.Peek(), static_cast<std::byte>(str1.front()));
    EXPECT_EQ(buf.ReadableString(), str1);

    // `"1"` → `"12"`
    const std::string str2 {"2"};
    buf.Append(str2);
    EXPECT_EQ(buf.ReadableString(), str1 + str2);

    // `"12"` → `"123"`
    const std::string str3 {"3"};
    buf.Append(Buffer {str3});
    EXPECT_EQ(buf.ReadableString(), str1 + str2 + str3);

    if (const auto total_size {str1.length() + str2.length() + str3.length()};
        init_size >= total_size) {
        EXPECT_EQ(buf.WritableSize(), init_size - total_size);
    } else {
        EXPECT_EQ(buf.WritableSize(), 0);
    }

    // `"123"` → `""`
    EXPECT_EQ(buf.RetrieveAllToString(), str1 + str2 + str3);
    EXPECT_TRUE(buf.Empty());

    // Add a new-line character when appending a string.
    buf.Append("hello", NewLine::CRLF);
    EXPECT_EQ(buf.RetrieveAllToString(), "hello\r\n");
    EXPECT_TRUE(buf.Empty());
    buf.Append("hello", NewLine::LF);
    EXPECT_EQ(buf.RetrieveAllToString(), "hello\n");
}

TEST(BufferTest, Clear) {
    Buffer buf {"hello"};
    buf.Clear();
    EXPECT_TRUE(buf.Empty());
    EXPECT_EQ(buf.ReadableSize(), 0);
    EXPECT_TRUE(buf.ReadableString().empty());
    EXPECT_EQ(buf.Peek(), std::nullopt);
}

TEST(IOBufferTest, ReadWrite) {
    IOBuffer buf;
    EXPECT_TRUE(buf.Empty());

    // `""` → `"hello"`
    const std::string str {"hello"};
    std::stringstream ss {str};
    io::StringStream io {ss, ss};
    EXPECT_EQ(buf.ReadFrom(io), str.length());
    EXPECT_EQ(buf.ReadableString(), str);

    ss.clear();
    ss.str("");
    EXPECT_TRUE(ss.str().empty());

    // `"hello"` → `""`
    EXPECT_EQ(buf.WriteTo(io), str.length());
    EXPECT_TRUE(buf.Empty());
    EXPECT_EQ(ss.str(), str);
}