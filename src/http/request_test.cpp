#include "request.h"

#include <gtest/gtest.h>

using namespace ws;
using namespace ws::http;


TEST(HTTPRequestTest, Parse) {
    {
        Buffer buf {"POST /path/to/file HTTP/1.1\r\n"
                    "Host: server.id\r\n"
                    "Content-Type: application/x-www-form-urlencoded\r\n"
                    "Content-Length: 0\r\n"
                    "\r\n"};

        Request request {buf};
        EXPECT_EQ(request.Header("Content-Length"), "0");

        EXPECT_EQ(request.PostSize(), 0);
    }

    {
        Buffer buf {"POST /file HTTP/1.1\r\n"
                    "Host: server.id\r\n"
                    "Content-Type: application/x-www-form-urlencoded\r\n"
                    "Content-Length: 4\r\n"
                    "\r\n"
                    "id=1"};

        Request request {buf};
        EXPECT_FALSE(request.KeepAlive());
        EXPECT_EQ(request.Version(), "1.1");
        EXPECT_EQ(request.Path(), "/file");
        EXPECT_EQ(request.Method(), Method::Post);

        EXPECT_EQ(request.Header("Content-Length"), "4");
        EXPECT_EQ(request.Header("Content-Type"),
                  "application/x-www-form-urlencoded");
        EXPECT_EQ(request.Header("Host"), "server.id");
        EXPECT_FALSE(request.Header("Connection"));

        EXPECT_EQ(request.Post("id"), "1");
        EXPECT_FALSE(request.Post("name"));

        // The query is case-sensitive.
        EXPECT_FALSE(request.Post("ID"));
        EXPECT_FALSE(request.Header("host"));
        EXPECT_FALSE(request.Header("HOST"));

        EXPECT_EQ(request.PostSize(), 1);
    }

    {
        Buffer buf {"POST /path/to/file HTTP/1.1\r\n"
                    "Host: server.id\r\n"
                    "Connection: keep-alive\r\n"
                    "Content-Type: application/x-www-form-urlencoded\r\n"
                    "Content-Length: 32\r\n"
                    "\r\n"
                    "id=1&name=mike+chen&msg=hello%21"};

        Request request {buf};
        EXPECT_TRUE(request.KeepAlive());
        EXPECT_EQ(request.Version(), "1.1");
        EXPECT_EQ(request.Path(), "/path/to/file");
        EXPECT_EQ(request.Method(), Method::Post);

        EXPECT_EQ(request.Header("Content-Length"), "32");
        EXPECT_EQ(request.Header("Content-Type"),
                  "application/x-www-form-urlencoded");
        EXPECT_EQ(request.Header("Host"), "server.id");

        EXPECT_EQ(request.Post("id"), "1");
        EXPECT_EQ(request.Post("name"), "mike chen");
        EXPECT_EQ(request.Post("msg"), "hello!");

        EXPECT_EQ(request.PostSize(), 3);
    }

    {
        Buffer buf {"POST /path/to/file HTTP/1.1\r\n"
                    "Host: server.id\r\n"
                    "Content-Type: application/x-www-form-urlencoded\r\n"
                    "Content-Length: 4\r\n"
                    "invalid body without an empty line"};

        Request request;
        EXPECT_THROW(request.Parse(buf), std::invalid_argument);
    }
}