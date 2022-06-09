#include "config.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace ws;
using namespace ws::cfg;


TEST(ConfigurationVariableConverterTest, Number) {
    EXPECT_EQ((VarConverter<std::string, int> {}("0")), 0);
    EXPECT_EQ((VarConverter<int, std::string> {}(0)), "0");

    EXPECT_DOUBLE_EQ((VarConverter<std::string, double> {}("10.1")), 10.1);

    // Cannot use `EXPECT_EQ` because the returned string may end with several '0's.
    EXPECT_TRUE(
        (VarConverter<double, std::string> {}(10.1)).starts_with("10.1"));

    EXPECT_THROW((VarConverter<std::string, int> {}("hello")),
                 std::invalid_argument);
}

TEST(ConfigurationVariableConverterTest, String) {
    EXPECT_EQ((VarConverter<std::string, std::string> {}("hello")), "hello");
}

TEST(ConfigurationVariableConverterTest, List) {
    EXPECT_EQ((VarConverter<std::string, std::list<int>> {}("[0, 1]")),
              (std::list<int> {0, 1}));

    EXPECT_EQ((VarConverter<std::string, std::vector<int>> {}("[0, 1]")),
              (std::vector<int> {0, 1}));

    EXPECT_EQ((VarConverter<std::string, std::vector<std::vector<int>>> {}(
                  "[[0], [1, 2]]")),
              (std::vector<std::vector<int>> {{0}, {1, 2}}));
}

TEST(ConfigurationVariableConverterTest, Set) {
    EXPECT_EQ(
        (VarConverter<std::string, std::unordered_set<int>> {}("[0, 0, 1]")),
        (std::unordered_set<int> {0, 1}));

    EXPECT_EQ((VarConverter<std::string, std::set<int>> {}("[0, 0, 1]")),
              (std::set<int> {0, 1}));

    EXPECT_EQ((VarConverter<std::string, std::set<std::set<int>>> {}(
                  "[[0], [0], [1, 2]]")),
              (std::set<std::set<int>> {{0}, {1, 2}}));
}

TEST(ConfigurationVariableConverterTest, Map) {
    EXPECT_EQ(
        (VarConverter<std::string, std::unordered_map<std::string, int>> {}(
            "{x: 0, y: 1}")),
        (std::unordered_map<std::string, int> {{"x", 0}, {"y", 1}}));

    EXPECT_EQ((VarConverter<std::string, std::map<std::string, int>> {}(
                  "{x: 0, y: 1}")),
              (std::map<std::string, int> {{"x", 0}, {"y", 1}}));

    EXPECT_EQ((VarConverter<std::string, std::map<std::string, int>> {}(
                  "{[0, 1]: 1}")),
              (std::map<std::string, int> {{"[0, 1]", 1}}));

    EXPECT_EQ(
        (VarConverter<std::string,
                      std::map<std::string, std::map<std::string, int>>> {}(
            "{f: {x: 0}, g: {x: 1, y: 2}}")),
        (std::map<std::string, std::map<std::string, int>> {
            {"f", {{"x", 0}}}, {"g", {{"x", 1}, {"y", 2}}}}));
}

TEST(ConfigurationVariableTest, Construction) {
    constexpr std::string_view name {"port"};
    constexpr std::string_view description {"The port number"};
    constexpr std::uint16_t val {80};

    const Var<std::uint16_t> var {name, val, description};
    EXPECT_EQ(var.Value(), val);
    EXPECT_EQ(var.Name(), name);
    EXPECT_EQ(var.Description(), description);
}

TEST(ConfigurationVariableTest, ConvertedFromString) {
    {
        // A scalar variable.
        Var<int> var {"", 0};
        constexpr int scalar {2};
        var.FromString(VarConverter<int, std::string> {}(scalar));
        EXPECT_EQ(var.Value(), scalar);
    }

    {
        // A list variable.
        Var<std::vector<int>> var {"", {}};
        const std::vector<int> list {1, 2};
        var.FromString(VarConverter<std::vector<int>, std::string> {}(list));
        EXPECT_EQ(var.Value(), list);
    }

    {
        // A set variable.
        Var<std::set<int>> var {"", {}};
        const std::set<int> set {1, 2};
        var.FromString(VarConverter<std::set<int>, std::string> {}(set));
        EXPECT_EQ(var.Value(), set);
    }

    {
        // A map variable.
        Var<std::map<std::string, int>> var {"", {}};
        const std::map<std::string, int> map {{"x", 0}, {"y", 1}};
        var.FromString(
            VarConverter<std::map<std::string, int>, std::string> {}(map));
        EXPECT_EQ(var.Value(), map);
    }
}

TEST(ConfigurationVariableTest, Listen) {
    class Listener {
    public:
        MOCK_METHOD(void, OnChange, (const int& old_val, const int& new_val),
                    (const, noexcept));
    };

    constexpr int old_val {0};
    constexpr int new_val {1};
    Var<int> var {"", old_val};

    // Add a listener for `var`.
    const Listener listener;
    const auto key {var.AddListener(
        [&listener](const int& old_val, const int& new_val) noexcept {
            listener.OnChange(old_val, new_val);
        })};

    // When `var` changes, the listener will be notified.
    EXPECT_CALL(listener, OnChange(old_val, new_val)).Times(1);
    var.SetValue(1);

    // A listener can be removed by its key.
    // When the value changes again, removed listeners will not be notified.
    EXPECT_CALL(listener, OnChange).Times(0);
    var.RemoveListener(key);
    var.SetValue(2);
}

TEST(ConfigurationTest, Lookup) {
    Config cfg {"test"};

    // The variable `x` does not exist in the current configuration.
    // `LoadYaml` will not create it.
    const auto node {LoadYamlString("{x: 1}")};
    cfg.LoadYaml(node);
    EXPECT_FALSE(cfg.Lookup<int>("x"));

    // Create a variable `x`.
    cfg.Lookup<int>("x", 0);
    const auto x {cfg.Lookup<int>("x")};
    EXPECT_TRUE(x);
    if (x) {
        EXPECT_EQ(x->Value(), 0);
    }

    // Set `x` from a `YAML` node.
    cfg.LoadYaml(node);
    EXPECT_TRUE(x);
    if (x) {
        EXPECT_EQ(x->Value(), 1);
    }

    // The variable type is mismatched.
    EXPECT_THROW(cfg.Lookup<wchar_t>("x"), std::invalid_argument);
}

TEST(ConfigurationTest, Visit) {
    class Visitor {
    public:
        MOCK_METHOD(void, OnVisit, (int val), (const, noexcept));
    };

    Config cfg {"test"};
    const std::array vals {0, 1};
    const Visitor visitor;
    for (const auto val : vals) {
        cfg.Lookup<int>(std::to_string(val), val);
        EXPECT_CALL(visitor, OnVisit(val)).Times(1);
    }

    cfg.Visit([&visitor](const VarBase::Ptr base) noexcept {
        const auto var {std::dynamic_pointer_cast<Var<int>>(base)};
        assert(var);
        visitor.OnVisit(var->Value());
    });
}