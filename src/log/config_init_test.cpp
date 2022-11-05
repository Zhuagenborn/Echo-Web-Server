#include "config_init.h"
#include "log.h"

#include <gtest/gtest.h>

#include <cassert>

using namespace testing;

using ws::log::AppenderConfig;
using ws::log::LoggerConfig;

using ws::cfg::Config;
using ws::log::AppenderType;
using ws::log::Formatter;
using ws::log::Level;
using ws::log::Manager;


TEST(LoggerConfigurationTest, Construction) {
    constexpr std::string_view yaml {R"(
- name: root
  level: info
  capacity: 50
  appenders:
    - type: stdout
- name: system
  level: debug
  formatter: "%d"
  appenders:
    - type: stdout
    - type: stdout
)"};

    const std::unordered_set<std::string> logger_names {"root", "system"};
    Manager::Ptr manager {std::make_shared<Manager>("test")};
    Config config {"test"};
    const auto loggers {SetListener(
        config.Lookup("loggers", std::unordered_set<LoggerConfig> {},
                      "Loggers"),
        manager)};
    loggers->FromString(yaml);

    // Check whether the configuration has been set.
    for (const auto& cfg : loggers->GetValue()) {
        EXPECT_TRUE(logger_names.contains(cfg.name));

        if (cfg.name == "root") {
            EXPECT_EQ(cfg.level, Level::Info);
            EXPECT_EQ(cfg.capacity, 50);
            EXPECT_TRUE(cfg.formatter.empty());
            EXPECT_EQ(
                cfg.appenders,
                (std::list<AppenderConfig> {{.type = AppenderType::StdOut}}));

        } else if (cfg.name == "system") {
            EXPECT_EQ(cfg.level, Level::Debug);
            EXPECT_EQ(cfg.capacity, 0);
            EXPECT_EQ(cfg.formatter, "%d");
            EXPECT_EQ(cfg.appenders, (std::list<AppenderConfig> {
                                         {.type = AppenderType::StdOut},
                                         {.type = AppenderType::StdOut}}));
        }
    }

    // Check whether the loggers have been set.
    for (const auto& cfg : loggers->GetValue()) {
        if (logger_names.contains(cfg.name)) {
            const auto logger {manager->FindLogger(cfg.name)};
            EXPECT_TRUE(logger);
            if (logger) {
                EXPECT_EQ(logger->Name(), cfg.name);
                if (cfg.name == "root") {
                    EXPECT_EQ(logger->GetLevel(), Level::Info);
                    EXPECT_EQ(logger->Capacity(), 50);
                    EXPECT_EQ(logger->GetDefaultFormatter()->Pattern(),
                              Formatter::Default()->Pattern());
                } else if (cfg.name == "system") {
                    EXPECT_EQ(logger->GetLevel(), Level::Debug);
                    EXPECT_EQ(logger->Capacity(), 0);
                    EXPECT_EQ(logger->GetDefaultFormatter()->Pattern(), "%d");
                } else {
                    assert(false);
                }
            }
        }
    }
}