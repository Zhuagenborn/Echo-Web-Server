#include "io.h"
#include "containers/buffer.h"
#include "test_util.h"
#include "util.h"

#include <gtest/gtest.h>

#include <fcntl.h>

#include <sstream>
#include <string>

using namespace ws;
using namespace ws::test;


TEST(StringStreamIOTest, ReadWrite) {
    std::stringstream ss;
    io::StringStream io {ss, ss};
    EXPECT_TRUE(ss.str().empty());

    // Buffer: `"hello"` → `""`
    // IO: `""` → `"hello"`
    const std::string str {"hello"};
    IOBuffer buf {str};
    EXPECT_EQ(io.ReadFrom(buf), str.length());
    EXPECT_TRUE(buf.Empty());
    EXPECT_EQ(ss.str(), str);

    // Buffer: `""` → `"hello"`
    EXPECT_EQ(io.WriteTo(buf), str.length());
    EXPECT_EQ(buf.ReadableString(), str);
}

TEST(FileDescriptorIOTest, ReadWrite) {
    const auto [write_fd, path] {CreateTempTestFile()};
    const auto read_fd {open(path.c_str(), O_RDONLY)};

    // Clean the file.
    const RAII file_raii {path, [](const auto& path) noexcept {
                              unlink(path.c_str());
                          }};

    // Close file descriptors.
    const RAII fd_raii {std::pair {read_fd, write_fd},
                        [](const auto& fds) noexcept {
                            close(fds.first);
                            close(fds.second);
                        }};

    const std::string str {"hello"};
    Buffer buf {str};
    EXPECT_FALSE(buf.Empty());

    io::FileDescriptor io {read_fd, write_fd};
    io.ReadFrom(buf);
    EXPECT_TRUE(buf.Empty());

    io.WriteTo(buf);
    EXPECT_EQ(buf.ReadableString(), str);

    // Throw an exception if a descriptor is invalid.
    io::FileDescriptor invalid_io {invalid_file_descriptor,
                                   invalid_file_descriptor};
    EXPECT_THROW(invalid_io.WriteTo(buf), std::system_error);
    EXPECT_THROW(invalid_io.ReadFrom(buf), std::system_error);
}