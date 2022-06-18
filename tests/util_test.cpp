#include "util.h"
#include "containers/buffer.h"
#include "io.h"
#include "test_util.h"

#include <gtest/gtest.h>

using namespace ws;
using namespace ws::test;


//! A test type for singleton support.
class Type {
public:
    constexpr Type() noexcept = default;

    constexpr Type(const int v1) noexcept : vals_ {v1, 0} {}

    constexpr Type(const int v1, const int v2) noexcept : vals_ {v1, v2} {}

    constexpr std::pair<int, int> Vals() const noexcept {
        return vals_;
    }

private:
    std::pair<int, int> vals_;
};

TEST(StringTest, LetterCaseConversion) {
    EXPECT_EQ(StringToLower("HELLO"), "hello");
    EXPECT_EQ(StringToUpper("hello"), "HELLO");

    EXPECT_EQ(StringToLower(""), "");
    EXPECT_EQ(StringToUpper(""), "");
}

TEST(StringTest, ReplaceString) {
    // No matched sub-strings.
    EXPECT_EQ(ReplaceAllSubstring("", "a", "A"), "");
    EXPECT_EQ(ReplaceAllSubstring("b", "a", "A"), "b");

    EXPECT_EQ(ReplaceAllSubstring("a", "a", ""), "");
    EXPECT_EQ(ReplaceAllSubstring("a b", "a", "A"), "A b");
    EXPECT_EQ(ReplaceAllSubstring("a a", "a", "A"), "A A");
    EXPECT_EQ(ReplaceAllSubstring("a aa b", "a", "A"), "A AA b");
}

TEST(StringTest, SplitString) {
    // No matched delimiter.
    EXPECT_EQ(SplitString("", std::regex {"\\s+"}),
              std::vector<std::string> {""});
    EXPECT_EQ(SplitString("a", std::regex {"\\s+"}),
              std::vector<std::string> {"a"});

    EXPECT_EQ(SplitString("a ", std::regex {"\\s+"}),
              std::vector<std::string> {"a"});
    EXPECT_EQ(SplitString("a b", std::regex {"\\s+"}),
              (std::vector<std::string> {"a", "b"}));
    EXPECT_EQ(SplitString("a,b", std::regex {","}),
              (std::vector<std::string> {"a", "b"}));
}

TEST(StringTest, SplitStringToLines) {
    // No matched delimiter.
    EXPECT_EQ(SplitStringToLines(""), std::vector<std::string> {""});
    EXPECT_EQ(SplitStringToLines("a"), std::vector<std::string> {"a"});

    // Match `LF`.
    EXPECT_EQ(SplitStringToLines("a\n"), std::vector<std::string> {"a"});
    EXPECT_EQ(SplitStringToLines("a\nb"),
              (std::vector<std::string> {"a", "b"}));

    // Match `CRLF`.
    EXPECT_EQ(SplitStringToLines("a\r\nb"),
              (std::vector<std::string> {"a", "b"}));
}

TEST(SingletonTest, Construction) {
    using DefaultSingletonType = Singleton<Type>;

    // The instance is `Type{}`.
    EXPECT_EQ(&DefaultSingletonType::Instance(),
              &DefaultSingletonType::Instance());
    EXPECT_EQ(DefaultSingletonType::Instance().Vals(),
              (std::pair<int, int> {}));

    using OneParamSingletonType = Singleton<Type, int>;

    // The instance is `Type{0}`.
    EXPECT_EQ(&OneParamSingletonType::Instance<0>(),
              &OneParamSingletonType::Instance<0>());

    EXPECT_NE(&OneParamSingletonType::Instance<0>(),
              &OneParamSingletonType::Instance<1>());

    using TwoParamSingletonType = Singleton<Type, int, int>;

    // The instance is `Type{0, 0}`.
    EXPECT_EQ((&TwoParamSingletonType::Instance<0, 0>()),
              (&TwoParamSingletonType::Instance<0, 0>()));

    EXPECT_NE((&TwoParamSingletonType::Instance<0, 0>()),
              (&TwoParamSingletonType::Instance<1, 1>()));

    // The instance is `Type{1, 1}`.
    EXPECT_EQ((TwoParamSingletonType::Instance<1, 1>().Vals()),
              (std::pair<int, int> {1, 1}));
}

TEST(SingletonPtrTest, Construction) {
    using DefaultSingletonPtrType = SingletonPtr<Type>;

    // The instance is `Type{}`.
    EXPECT_EQ(DefaultSingletonPtrType::Instance().get(),
              DefaultSingletonPtrType::Instance().get());

    EXPECT_EQ(DefaultSingletonPtrType::Instance()->Vals(),
              (std::pair<int, int> {}));

    using OneParamSingletonPtrType = SingletonPtr<Type, int>;

    // The instance is `Type{0}`;
    EXPECT_EQ(OneParamSingletonPtrType::Instance<0>().get(),
              OneParamSingletonPtrType::Instance<0>().get());

    EXPECT_NE(OneParamSingletonPtrType::Instance<0>().get(),
              OneParamSingletonPtrType::Instance<1>().get());

    using TwoParamSingletonPtrType = SingletonPtr<Type, int, int>;

    // The instance is `Type{0, 0}`.
    EXPECT_EQ((TwoParamSingletonPtrType::Instance<0, 0>().get()),
              (TwoParamSingletonPtrType::Instance<0, 0>().get()));

    EXPECT_NE((TwoParamSingletonPtrType::Instance<0, 0>().get()),
              (TwoParamSingletonPtrType::Instance<1, 1>().get()));

    // The instance is `Type{1, 1}`.
    EXPECT_EQ((TwoParamSingletonPtrType::Instance<1, 1>()->Vals()),
              (std::pair<int, int> {1, 1}));
}

TEST(RAIITest, Destroy) {
    int val {0};

    {
        RAII raii {std::ref(val), [](int& val) noexcept {
                       val += 1;
                   }};

        EXPECT_EQ(raii.Object(), val);
    }

    // The value of `val` should be increased after RAII was destroyed.
    EXPECT_EQ(val, 1);
}

TEST(LoadYamlStringTest, RequiredField) {
    EXPECT_NO_THROW(LoadYamlString("id: 0"));
    EXPECT_NO_THROW(LoadYamlString("id: 0", {"id"}));

    // `name` is not in the object `{id: 0}`.
    EXPECT_THROW(LoadYamlString("id: 0", {"name"}), std::invalid_argument);
}

TEST(YamlNodeTest, ThrowIfFieldIsNotScalar) {
    EXPECT_NO_THROW(
        (ThrowIfYamlFieldIsNotScalar(LoadYamlString("id: 0"), "id")));

    EXPECT_THROW((ThrowIfYamlFieldIsNotScalar(LoadYamlString("id: 0"), "name")),
                 std::invalid_argument);

    // An array is not scalar.
    EXPECT_THROW((ThrowIfYamlFieldIsNotScalar(LoadYamlString("id: [0]"), "id")),
                 std::invalid_argument);
}

TEST(ConceptTest, Addable) {
    // `int` + `int` = `int`
    EXPECT_TRUE((Addable<int, int>));
    EXPECT_TRUE((Addable<int, int, int>));

    // `std::string` + `char` = `std::string`
    EXPECT_TRUE((Addable<std::string, char>));
    EXPECT_TRUE((Addable<std::string, char, std::string>));

    // `std::string` + `char` ≠ `char`
    EXPECT_FALSE((Addable<std::string, char, char>));
}

TEST(MappedReadOnlyFile, Map) {
    {
        // The path refers to a directory.
        MappedReadOnlyFile file;
        EXPECT_THROW(file.Map("."), std::invalid_argument);
    }

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

        MappedReadOnlyFile file;
        const auto map_base {file.Map(path)};
        EXPECT_TRUE(map_base);
        EXPECT_EQ(file.Data(), map_base);
        EXPECT_EQ(
            (std::string_view {reinterpret_cast<char*>(map_base), file.Size()}),
            data);

        EXPECT_EQ(file.Path(), path);
        EXPECT_EQ(file.Size(), data.length());

        file.Unmap();
        EXPECT_FALSE(file.Data());
    }
}