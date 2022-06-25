#include "response.h"
#include "io.h"
#include "test_util.h"

#include <gtest/gtest.h>

using namespace ws;
using namespace ws::http;
using namespace ws::test;


namespace {

constexpr std::string_view system_error_tip {
    "The unit test involves system exceptions which may have different "
    "explanatory strings on platforms.\n"
    "Please check that firstly if the test failed."};
}

TEST(HTTPResponseTest, BuiltByFile) {
    {
        // Create a temporary file.
        const auto [fd, path] {CreateTempTestFile()};
        const RAII raii {std::pair {fd, path}, [](const auto& file) noexcept {
                             close(file.first);
                             unlink(file.second.c_str());
                         }};

        // Write data to the temporary file, otherwise it cannot be mapped.
        constexpr std::string_view data {"hello"};
        Buffer str {data};
        io::FileDescriptor io {invalid_file_descriptor, fd};
        io.ReadFrom(str);

        Buffer buf;
        StatusCode status_code {StatusCode::OK};
        Response header {""};
        header.SetKeepAlive(true);
        EXPECT_TRUE(header.Build(buf, path, status_code));
        EXPECT_EQ(status_code, StatusCode::OK);

        constexpr std::string_view content {
            "HTTP/1.1 200 OK\r\n"
            "Connection: keep-alive\r\n"
            "keep-alive: max=6, timeout=120\r\n"
            "Content-type: application/octet-stream\r\n"
            "Content-length: 5\r\n"
            "\r\n"};
        EXPECT_EQ(buf.RetrieveAllToString(), content);
    }

    {
        Buffer buf;
        StatusCode status_code {StatusCode::OK};
        Response header {std::filesystem::current_path()};
        EXPECT_FALSE(header.Build(buf, "non_existing_file", status_code));
        EXPECT_EQ(status_code, StatusCode::BadRequest);

        constexpr std::string_view content {
            "HTTP/1.1 400 Bad Request\r\n"
            "Connection: close\r\n"
            "Content-type: text/html\r\n"
            "Content-length: 114\r\n"
            "\r\n"
            "<html>\r\n"
            "<title>ERROR</title>\r\n"
            "<body>\r\n"
            "<p>400 : Bad Request</p>\r\n"
            "<p>No such file or directory</p>\r\n"
            "</body>\r\n"
            "</html>"};
        EXPECT_EQ(buf.RetrieveAllToString(), content) << system_error_tip;
    }
}

TEST(HTTPResponseTest, BuiltByPredefinedErrorContent) {
    // Failed to find HTML template pages because the root directory is not set.
    Buffer buf;
    Response header {""};
    header.Build(buf, StatusCode::OK, "hello");

    constexpr std::string_view content {"HTTP/1.1 400 Bad Request\r\n"
                                        "Connection: close\r\n"
                                        "Content-type: text/html\r\n"
                                        "Content-length: 114\r\n"
                                        "\r\n"
                                        "<html>\r\n"
                                        "<title>ERROR</title>\r\n"
                                        "<body>\r\n"
                                        "<p>400 : Bad Request</p>\r\n"
                                        "<p>No such file or directory</p>\r\n"
                                        "</body>\r\n"
                                        "</html>"};
    EXPECT_EQ(buf.RetrieveAllToString(), content) << system_error_tip;
}