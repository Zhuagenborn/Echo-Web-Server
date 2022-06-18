#include "http.h"

#include <gtest/gtest.h>

using namespace ws::http;


TEST(HTTPTest, StatusCodeEnumConversion) {
    EXPECT_EQ(StatusCodeToMessage(StatusCode::OK), "OK");
    EXPECT_EQ(StatusCodeToMessage(StatusCode::Forbidden), "Forbidden");

    EXPECT_EQ(StatusCodeToInteger(StatusCode::OK), 200);
    EXPECT_EQ(StatusCodeToInteger(StatusCode::Forbidden), 403);
}

TEST(HTTPTest, MethodEnumConversion) {
    EXPECT_EQ(MethodToString(Method::Post), "POST");
    EXPECT_EQ(MethodToString(Method::Get), "GET");

    EXPECT_EQ(StringToMethod("GET"), Method::Get);
    EXPECT_EQ(StringToMethod("get"), Method::Get);
    EXPECT_THROW(StringToMethod("Unknown"), std::invalid_argument);
}

TEST(HTTPTest, ContentType) {
    EXPECT_EQ(ContentTypeByFileName("x.PNG"), "image/png");
    EXPECT_EQ(ContentTypeByFileName("x.jpg"), "image/jpeg");
    EXPECT_EQ(ContentTypeByFileName("unknown"), "application/octet-stream");
    EXPECT_EQ(ContentTypeByFileName("x.unknown"), "application/octet-stream");

    EXPECT_EQ(ContentTypeByFileName("path/to/x.txt"), "text/plain");
    EXPECT_EQ(ContentTypeByFileName("path/to/unknown"),
              "application/octet-stream");
}

TEST(HTTPTest, URLEncoding) {
    EXPECT_EQ(DecodeURLEncodedCharacter("%20"), ' ');
    EXPECT_EQ(DecodeURLEncodedCharacter("%21"), '!');
    EXPECT_EQ(DecodeURLEncodedCharacter("%25"), '%');

    EXPECT_THROW(DecodeURLEncodedCharacter(""), std::invalid_argument);
    EXPECT_THROW(DecodeURLEncodedCharacter("20"), std::invalid_argument);
    EXPECT_THROW(DecodeURLEncodedCharacter("%123"), std::invalid_argument);

    EXPECT_EQ(DecodeURLEncodedString(""), "");
    EXPECT_EQ(DecodeURLEncodedString("hello"), "hello");
    EXPECT_EQ(DecodeURLEncodedString("hello%20world"), "hello world");
    EXPECT_EQ(DecodeURLEncodedString("go%21"), "go!");
    EXPECT_EQ(DecodeURLEncodedString("%25"), "%");

    EXPECT_THROW(DecodeURLEncodedString("hello%2"), std::invalid_argument);
}

TEST(HTTPTest, HTMLPlaceholder) {
    EXPECT_EQ(HTMLPlaceholder("name"), "<$name$>");
    EXPECT_EQ(HTMLPlaceholder("id"), "<$id$>");
}

TEST(HTTPTest, PutParameterIntoHTML) {
    {
        // Ignore parameters that do not have a corresponding placeholder in the template.
        const Parameters params {{"name", "mike"}};
        EXPECT_EQ(PutParamIntoHTML("", params), "");
    }

    {
        std::string html_template {"<html>\r\n"
                                   "<body>\r\n"
                                   "<p><$name$> said <$msg$></p>\r\n"
                                   "</body>\r\n"
                                   "</html>"};

        constexpr std::string_view html {"<html>\r\n"
                                         "<body>\r\n"
                                         "<p>mike said <$msg$></p>\r\n"
                                         "</body>\r\n"
                                         "</html>"};

        // Replace only some of the placeholders.
        const Parameters params {{"name", "mike"}};
        EXPECT_EQ(PutParamIntoHTML(std::move(html_template), params), html);
    }

    {
        std::string html_template {"<html>\r\n"
                                   "<body>\r\n"
                                   "<p><$name$> said <$msg$>, <$msg$></p>\r\n"
                                   "</body>\r\n"
                                   "</html>"};

        constexpr std::string_view html {"<html>\r\n"
                                         "<body>\r\n"
                                         "<p>mike said hello, hello</p>\r\n"
                                         "</body>\r\n"
                                         "</html>"};

        // Replace all placeholders.
        const Parameters params {{"name", "mike"}, {"msg", "hello"}};
        EXPECT_EQ(PutParamIntoHTML(std::move(html_template), params), html);
    }
}