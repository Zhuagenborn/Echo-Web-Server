#include "log.h"
#include "config_init.h"
#include "field.h"

#include <algorithm>
#include <cassert>
#include <stdexcept>


namespace ws {

namespace log {

namespace {
constexpr std::string_view root_logger_name {"root"};
}

std::string_view LevelToString(const Level level) noexcept {
    static const std::unordered_map<Level, std::string_view> levels {
        {Level::Debug, "Debug"},
        {Level::Info, "Info"},
        {Level::Warn, "Warn"},
        {Level::Error, "Error"},
        {Level::Fatal, "Fatal"}};

    return levels.at(level);
}

std::string to_string(const Level level) noexcept {
    return LevelToString(level).data();
}

Level StringToLevel(std::string str) {
    static const std::unordered_map<std::string_view, Level> levels {
        {"DEBUG", Level::Debug},
        {"INFO", Level::Info},
        {"WARN", Level::Warn},
        {"ERROR", Level::Error},
        {"FATAL", Level::Fatal}};

    str = StringToUpper(str);
    if (const auto level {levels.find(str)}; level != levels.cend()) {
        return level->second;
    } else {
        throw std::invalid_argument {
            fmt::format("Invalid log level: '{}'", str)};
    }
}

Event::Ptr Event::Create(const log::Level level,
                         std::experimental::source_location location,
                         const std::uint32_t thread_id,
                         Clock::time_point time) noexcept {
    // Make `std::make_shared` have the access to the private constructor.
    struct MakeSharedEvent : public Event {
        MakeSharedEvent(const log::Level level,
                        std::experimental::source_location location,
                        const std::uint32_t thread_id,
                        Clock::time_point time) noexcept :
            Event {level, std::move(location), thread_id, std::move(time)} {}
    };

    return std::make_shared<MakeSharedEvent>(level, std::move(location),
                                             thread_id, std::move(time));
}

Event::Event(const log::Level level,
             const std::experimental::source_location location,
             const std::uint32_t thread_id, Clock::time_point time) noexcept :
    level_ {level},
    file_name_ {location.file_name()},
    line_num_ {location.line()},
    thread_id_ {thread_id},
    time_ {std::move(time)} {}

log::Level Event::Level() const noexcept {
    return level_;
}

std::string_view Event::FileName() const noexcept {
    return file_name_;
}

std::size_t Event::LineNum() const noexcept {
    return line_num_;
}

std::uint32_t Event::ThreadId() const noexcept {
    return thread_id_;
}

Event::Clock::time_point Event::Time() const noexcept {
    return time_;
}

std::string Event::Message() const noexcept {
    return msg_.str();
}

std::ostringstream& Event::MessageStream() noexcept {
    return msg_;
}

Event::Ptr operator<<(const Event::Ptr event,
                      const std::string_view msg) noexcept {
    event->MessageStream() << msg;
    return event;
}

EventWriter::EventWriter(Logger& logger, const Event::Ptr event) noexcept :
    logger_ {logger}, event_ {event} {}

EventWriter::~EventWriter() noexcept {
    logger_.Log(event_);
}

std::ostringstream& EventWriter::MessageStream() noexcept {
    return event_->MessageStream();
}

Formatter::Ptr Formatter::Default() noexcept {
    static constexpr std::string_view pattern {
        "%d{%Y-%m-%d %H:%M:%S}%T%t%T[%p]%T[%c]%T<%f:%l>%T%m%n"};
    static const auto ins {std::make_shared<Formatter>(pattern)};
    return ins;
}

Formatter::Field::Field(const std::string_view format) noexcept {}

Formatter::Formatter(const std::string_view pattern) :
    pattern_ {pattern},
    fields_ {field::RawFieldsToFormatFields(field::ParsePattern(pattern))} {}

std::string Formatter::Format(const Logger& logger,
                              const Event& event) const noexcept {
    std::ostringstream str;
    std::ranges::for_each(fields_, [&str, &logger, &event](auto& it) noexcept {
        it->Format(str, logger, event);
    });

    return str.str();
}

std::string_view Formatter::Pattern() const noexcept {
    return pattern_;
}

Logger::Logger(const std::string_view name, const log::Level level,
               const std::optional<std::size_t> capacity) noexcept :
    name_ {name}, level_ {level}, capacity_ {capacity.value_or(0)} {
    if (capacity_ > 0) {
        async_ = true;
        event_deque_ = std::make_unique<BlockDeque<Event::Ptr>>(capacity_);
        writer_thread_ =
            std::make_unique<std::thread>(&Logger::AsyncLogProc, this);
    } else {
        async_ = false;
    }
}

Logger::~Logger() noexcept {
    if (async_) {
        assert(event_deque_);
        event_deque_->Close();

        assert(writer_thread_ && writer_thread_->joinable());
        writer_thread_->join();
    }
}

void Logger::AsyncLogProc() noexcept {
    while (true) {
        if (const auto event {event_deque_->Pop()}; event) {
            SyncLog(*event.value());
        } else {
            return;
        }
    }
}

void Logger::SyncLog(const Event& event) noexcept {
    const std::lock_guard locker {mtx_};
    std::ranges::for_each(appenders_, [&event, this](auto& it) noexcept {
        it->Log(*this, event);
    });
}

void Logger::Log(const Event::Ptr event) noexcept {
    if (event->Level() >= level_) {
        if (async_) {
            const std::lock_guard locker {mtx_};
            event_deque_->PushBack(std::move(event));
        } else {
            SyncLog(*event);
        }
    }
}

void Logger::AddAppender(const Appender::Ptr appender) noexcept {
    const std::lock_guard locker {mtx_};
    if (!appender->GetFormatter()) {
        appender->SetFormatter(formatter_);
    }

    appenders_.push_back(appender);
}

/**
 * @todo Replace @p std::remove_if with @p std::ranges::remove_if.
 */
void Logger::RemoveAppender(const Appender::Ptr appender) noexcept {
    const std::lock_guard locker {mtx_};
    std::remove_if(
        appenders_.begin(), appenders_.end(),
        [&appender](const auto& it) noexcept { return it == appender; });
}

void Logger::ClearAppenders() noexcept {
    const std::lock_guard locker {mtx_};
    appenders_.clear();
}

log::Level Logger::GetLevel() const noexcept {
    const std::lock_guard locker {mtx_};
    return level_;
}

void Logger::SetLevel(const log::Level level) noexcept {
    const std::lock_guard locker {mtx_};
    level_ = level;
}

Formatter::Ptr Logger::GetDefaultFormatter() const noexcept {
    const std::lock_guard locker {mtx_};
    return formatter_;
}

void Logger::SetDefaultFormatter(const Formatter::Ptr formatter) noexcept {
    const std::lock_guard locker {mtx_};
    formatter_ = formatter;
    std::ranges::for_each(appenders_, [&formatter](const auto& it) noexcept {
        it->SetFormatter(formatter);
    });
}

void Logger::SetDefaultFormatter(const std::string_view pattern) {
    SetDefaultFormatter(std::make_shared<Formatter>(pattern));
}

std::size_t Logger::Capacity() const noexcept {
    return capacity_;
}

std::string_view Logger::Name() const noexcept {
    return name_;
}

std::string Logger::ToYamlString() const noexcept {
    const std::lock_guard locker {mtx_};

    YAML::Node node;
    node["name"] = name_;
    node["level"] = LevelToString(level_).data();

    if (formatter_) {
        node["formatter"] = formatter_->Pattern().data();
    }

    for (const auto& appender : appenders_) {
        node["appenders"].push_back(YAML::Load(appender->ToYamlString()));
    }

    std::ostringstream ss;
    ss << node;
    return ss.str();
}

void Log(const Logger::Ptr logger, Event::Ptr event) noexcept {
    if (logger) {
        logger->Log(std::move(event));
    }
}

Logger::Ptr operator<<(const Logger::Ptr logger, Event::Ptr event) noexcept {
    Log(logger, std::move(event));
    return logger;
}

Manager::Manager(const std::string_view name) noexcept : name_ {name} {}

std::string_view Manager::Name() const noexcept {
    return name_;
}

Logger::Ptr Manager::FindLogger(
    const std::string_view name, const log::Level level,
    const std::optional<std::size_t> capacity) noexcept {
    const std::lock_guard locker {mtx_};
    if (const auto logger {loggers_.find(name.data())};
        logger != loggers_.cend()) {
        return logger->second;
    } else {
        const auto new_logger {std::make_shared<Logger>(name, level, capacity)};
        loggers_.emplace(name, new_logger);
        return new_logger;
    }
}

void Manager::RemoveLogger(const std::string_view name) noexcept {
    const std::lock_guard locker {mtx_};
    loggers_.erase(name.data());
}

std::string Manager::ToYamlString() const noexcept {
    const std::lock_guard locker {mtx_};

    YAML::Node node;
    for (const auto& logger : loggers_) {
        node.push_back(YAML::Load(logger.second->ToYamlString()));
    }

    std::ostringstream ss;
    ss << node;
    return ss.str();
}

void Manager::InitConfig() noexcept {
    static std::once_flag init_flag;
    std::call_once(init_flag, []() noexcept {
        SetListener(
            cfg::RootConfig()->Lookup(
                "loggers", std::unordered_set<LoggerConfig> {}, "Loggers"),
            RootManager());
    });
}

Manager::Ptr RootManager() noexcept {
    static std::once_flag init_flag;
    static const auto ins {std::make_shared<Manager>("root")};
    std::call_once(init_flag, []() noexcept {
        ins->FindLogger(root_logger_name)
            ->AddAppender(std::make_shared<StdOutAppender>());
    });

    return ins;
}

Logger::Ptr RootLogger() noexcept {
    return RootManager()->FindLogger(root_logger_name);
}

Logger::Ptr FindLogger(const std::string_view name) noexcept {
    return RootManager()->FindLogger(name);
}

std::ostream& operator<<(std::ostream& os, const AppenderType type) noexcept {
    return os << AppenderTypeToString(type);
}

std::ostream& operator<<(std::ostream& os, const Level level) noexcept {
    return os << LevelToString(level);
}

}  // namespace log

}  // namespace ws