#include "log.h"

#include <gtest/gtest.h>

#include <array>
#include <stdexcept>

using namespace ws::log;

//! The fake appender that writes events to a string stream.
class FakeAppender : public Appender {
public:
    using Ptr = std::shared_ptr<FakeAppender>;

    using Appender::Appender;

    void Log(const Logger& logger, const Event& event) noexcept override {
        ss_ << formatter_->Format(logger, event);
    }

    //! Empty implementation.
    std::string ToYamlString() const noexcept override {
        return "";
    }

    //! Get the stored string and clear the stream.
    std::string Message() noexcept {
        const auto msg {ss_.str()};
        ss_.clear();
        ss_.str("");
        return msg;
    }

private:
    std::ostringstream ss_;
};

class LoggerTest : public testing::Test {
protected:
    Event::Ptr event_ {Event::Create(Level::Debug)};

    Formatter::Ptr formatter_ {Formatter::Default()};

    std::array<FakeAppender::Ptr, 2> appenders_ {
        std::make_shared<FakeAppender>(formatter_),
        std::make_shared<FakeAppender>(formatter_)};
};

TEST_F(LoggerTest, SynchronousLog) {
    Logger logger {"sync", Level::Debug};
    for (const auto& appender : appenders_) {
        logger.AddAppender(appender);
    }

    logger.Log(event_);
    const auto log {formatter_->Format(logger, *event_)};
    EXPECT_FALSE(log.empty());
    for (auto& appender : appenders_) {
        EXPECT_EQ(appender->Message(), log);
    }
}

TEST_F(LoggerTest, AsynchronousLog) {
    std::string log;

    {
        using namespace std::chrono_literals;

        Logger logger {"async", Level::Debug, 10};
        for (const auto& appender : appenders_) {
            logger.AddAppender(appender);
        }

        logger.Log(event_);
        log = formatter_->Format(logger, *event_);
        EXPECT_FALSE(log.empty());

        // Wait a short time for the logger to log events.
        std::this_thread::sleep_for(0.01s);
    }

    constexpr std::string_view test_tip {
        "It is normal that a logger may not have written all logs when it is "
        "closed"};
    for (auto& appender : appenders_) {
        EXPECT_EQ(appender->Message(), log) << test_tip;
    }
}

TEST(LogManagementTest, LoggerManager) {
    constexpr std::string_view logger_name {"logger"};
    constexpr std::string_view manager_name {"manager"};
    Manager manager {manager_name};
    EXPECT_EQ(manager.Name(), manager_name);

    const auto logger {manager.FindLogger(logger_name)};
    EXPECT_TRUE(logger);
    if (logger) {
        EXPECT_EQ(logger->Name(), logger_name);
    }
}

TEST(LogManagementTest, LevelEnumConversion) {
    EXPECT_EQ(LevelToString(Level::Debug), "Debug");
    EXPECT_EQ(StringToLevel("DEBUG"), Level::Debug);
    EXPECT_EQ(StringToLevel("debug"), Level::Debug);

    EXPECT_THROW(StringToLevel("unknown"), std::invalid_argument);
}

TEST(LogManagementTest, AppenderTypeEnumConversion) {
    EXPECT_EQ(AppenderTypeToString(AppenderType::StdOut), "StdOut");
    EXPECT_EQ(StringToAppenderType("stdout"), AppenderType::StdOut);
    EXPECT_EQ(StringToAppenderType("STDOUT"), AppenderType::StdOut);

    EXPECT_THROW(StringToAppenderType("unknown"), std::invalid_argument);
}

TEST(LogFormatterTest, Construction) {
    constexpr std::string_view valid_pattern {"%m"};
    const Formatter formatter {valid_pattern};
    EXPECT_EQ(formatter.Pattern(), valid_pattern);

    // Throw an exception if a pattern has invalid fields.
    constexpr std::string_view invalid_pattern {"%U"};
    EXPECT_THROW(Formatter {invalid_pattern}, std::invalid_argument);
}

TEST(LogFormatterTest, Format) {
    const Formatter formatter {"%m"};
    constexpr std::string_view msg {"hello"};
    const auto event {Event::Create(Level::Debug) << msg};
    EXPECT_EQ((formatter.Format(Logger {""}, *event)), msg);
}