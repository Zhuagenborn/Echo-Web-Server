#include "field.h"

#include <gtest/gtest.h>

using namespace ws::log::field;


TEST(LogFormatFieldTest, ParsePattern) {
    EXPECT_EQ(ParsePattern(""), std::list<RawField> {});

    EXPECT_EQ(ParsePattern("%d"), (std::list<RawField> {{false, "d", ""}}));

    EXPECT_EQ(ParsePattern("%d{yyyy-MM-dd}"),
              (std::list<RawField> {{false, "d", "yyyy-MM-dd"}}));

    EXPECT_EQ(ParsePattern("hello"),
              (std::list<RawField> {{true, "hello", ""}}));

    EXPECT_EQ(ParsePattern("[%t]%d"), (std::list<RawField> {{true, "[", ""},
                                                            {false, "t", ""},
                                                            {true, "]", ""},
                                                            {false, "d", ""}}));
}