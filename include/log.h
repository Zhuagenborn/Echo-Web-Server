/**
 * @file log.h
 * @brief The logging system.
 * @details
 * The supported tags and corresponding fields.
 * - @p m: Dispaly the event message.
 * - @p p: Dispaly the event level.
 * - @p t: Dispaly the thread ID.
 * - @p n: Insert a new line.
 * - @p c: Dispaly the logger name.
 * - @p d: Dispaly the event time.
 * - @p f: Dispaly the file name.
 * - @p l: Dispaly the line number.
 * - @p T: Insert a tab character.
 *
 * @author Chen Zhenshuo (chenzs108@outlook.com)
 * @par GitHub
 * https://github.com/Zhuagenborn
 * @version 1.0
 * @date 2022-05-17
 *
 * @example tests/log_test.cpp
 */

#pragma once

#include "config.h"
#include "containers/block_deque.h"
#include "util.h"

#include <chrono>
#include <experimental/source_location>
#include <fstream>
#include <iostream>
#include <list>
#include <memory>
#include <mutex>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <thread>
#include <unordered_map>


namespace ws::log {

class Logger;

enum class Level { Debug = 0, Info = 1, Warn = 2, Error = 3, Fatal = 4 };

//! Convert a level into a string.
std::string_view LevelToString(Level level) noexcept;

std::string to_string(Level level) noexcept;

std::ostream& operator<<(std::ostream& os, Level level) noexcept;

/**
 * @brief Convert a string into a level.
 *
 * @exception std::invalid_argument The string does not represent a level.
 */
Level StringToLevel(std::string str);

enum class AppenderType { StdOut = 0, File = 1 };

//! Convert an appender type into a string.
std::string_view AppenderTypeToString(AppenderType type) noexcept;

std::string to_string(AppenderType type) noexcept;

std::ostream& operator<<(std::ostream& os, AppenderType type) noexcept;

/**
 * @brief Convert a string into an appender type.
 *
 * @exception std::invalid_argument The string does not represent an appender type.
 */
AppenderType StringToAppenderType(std::string str);

/**
 * The log event.
 */
class Event {
public:
    using Ptr = std::shared_ptr<Event>;

    using Clock = std::chrono::system_clock;

    /**
     * @brief Create an event.
     *
     * @param level     The event level.
     * @param location  The event location.
     * @param thread_id The ID of the thread where the event occurred.
     * @param time      The event time.
     *
     * @todo Replace @p std::experimental::source_location with @p std::source_location.
     */
    static Ptr Create(log::Level level,
                      std::experimental::source_location location =
                          std::experimental::source_location::current(),
                      std::uint32_t thread_id = CurrentThreadId(),
                      Clock::time_point time = Clock::now()) noexcept;

    Event(const Event&) = delete;

    Event(Event&&) = delete;

    Event& operator=(const Event&) = delete;

    Event& operator=(Event&&) = delete;

    log::Level Level() const noexcept;

    std::string_view FileName() const noexcept;

    std::size_t LineNum() const noexcept;

    std::uint32_t ThreadId() const noexcept;

    Clock::time_point Time() const noexcept;

    //! Get the message written by the user.
    std::string Message() const noexcept;

    //! Get an output stream. The user can write messages into it.
    std::ostringstream& MessageStream() noexcept;

protected:
    explicit Event(log::Level level,
                   std::experimental::source_location location,
                   std::uint32_t thread_id, Clock::time_point time) noexcept;

private:
    log::Level level_;
    std::string_view file_name_;
    std::size_t line_num_;
    std::uint32_t thread_id_;
    Clock::time_point time_;

    std::ostringstream msg_;
};

Event::Ptr operator<<(Event::Ptr event, std::string_view msg) noexcept;

/**
 * The event formatter.
 */
class Formatter {
public:
    using Ptr = std::shared_ptr<Formatter>;

    static Formatter::Ptr Default() noexcept;

    /**
     * @interface Field
     * @brief
     * The field formatter for different information.
     * A formatter usually needs multiple field formatters, such as for thread ID and time.
     *
     * @see src/log/field.h
     */
    class Field {
    public:
        using Ptr = std::shared_ptr<Field>;

        /**
         * @brief Create a field formatter.
         *
         * @param format    An optional format required for some fields.
         */
        explicit Field(std::string_view format = "") noexcept;

        virtual ~Field() noexcept = default;

        //! Format a field into a string.
        virtual void Format(std::ostream& out, const Logger& logger,
                            const Event& event) noexcept = 0;

        /**
         * Get the tag representing a field.
         * The tag does not contain the preceding symbol @p %.
         */
        virtual std::string_view Tag() const noexcept = 0;
    };

    /**
     * @brief Create a formatter.
     *
     * @param pattern   A format pattern, consisting of multiple fields and their format.
     *
     * @exception std::invalid_argument The pattern is invalid.
     */
    explicit Formatter(std::string_view pattern);

    //! Format an event into a string.
    std::string Format(const Logger& logger, const Event& event) const noexcept;

    std::string_view Pattern() const noexcept;

private:
    std::string pattern_;
    std::list<Field::Ptr> fields_;
};

/**
 * The appender that writes events to a specific place.
 */
class Appender {
public:
    using Ptr = std::shared_ptr<Appender>;

    /**
     * @brief Create an appender with a format pattern.
     *
     * @exception std::invalid_argument The pattern is invalid.
     */
    explicit Appender(std::string_view pattern);

    explicit Appender(Formatter::Ptr formatter = Formatter::Default()) noexcept;

    Appender(const Appender&) = delete;

    Appender(Appender&&) = delete;

    Appender& operator=(const Appender&) = delete;

    Appender& operator=(Appender&&) = delete;

    virtual ~Appender() noexcept = default;

    //! Write an event into a specific place.
    virtual void Log(const Logger& logger, const Event& event) noexcept = 0;

    //! Convert the appender configuration into a @p YAML string for storage.
    virtual std::string ToYamlString() const noexcept = 0;

    Formatter::Ptr Formatter() const noexcept;

    void SetFormatter(Formatter::Ptr formatter) noexcept;

    /**
     * @brief Set a new formatter.
     *
     * @exception std::invalid_argument The format pattern is invalid.
     */
    void SetFormatter(std::string_view pattern);

protected:
    mutable std::mutex mtx_;
    Formatter::Ptr formatter_;
};

/**
 * The appender that writes events to the standard output stream.
 */
class StdOutAppender : public Appender {
public:
    using Ptr = std::shared_ptr<StdOutAppender>;

    using Appender::Appender;

    void Log(const Logger& logger, const Event& event) noexcept override;

    std::string ToYamlString() const noexcept override;
};

/**
 * The appender that writes events to a file.
 */
class FileAppender : public Appender {
public:
    using Ptr = std::shared_ptr<FileAppender>;

    /**
     * @brief Create a file appender.
     *
     * @param file_name A file name.
     * @param formatter A formatter.
     *
     * @exception std::system_error Failed to create the file.
     */
    explicit FileAppender(std::string_view file_name,
                          Formatter::Ptr formatter = Formatter::Default());

    void Log(const Logger& logger, const Event& event) noexcept override;

    std::string ToYamlString() const noexcept override;

private:
    std::string file_name_;
    std::ofstream file_;
};

/**
 * The logger, containing a list of appenders.
 * It can work synchronously or asynchronously.
 */
class Logger {
public:
    using Ptr = std::shared_ptr<Logger>;

    /**
     * @brief Create a logger.
     *
     * @param name  A name.
     * @param level
     * The lowest event level.
     * Any event with a level lower than it will not be processed.
     * @param capacity
     * The capacity of event queue.
     * If it is invalid or zero, the logger will be synchronous, otherwise asynchronous.
     * If the queue is full, the logging process will be blocked and waiting.
     */
    explicit Logger(
        std::string_view name, log::Level level = Level::Info,
        std::optional<std::size_t> capacity = std::nullopt) noexcept;

    ~Logger() noexcept;

    Logger(const Logger&) = delete;

    Logger(Logger&&) = delete;

    Logger& operator=(const Logger&) = delete;

    Logger& operator=(Logger&&) = delete;

    void Log(Event::Ptr event) noexcept;

    void AddAppender(Appender::Ptr appender) noexcept;

    void RemoveAppender(Appender::Ptr appender) noexcept;

    void ClearAppenders() noexcept;

    log::Level Level() const noexcept;

    void SetLevel(log::Level level) noexcept;

    Formatter::Ptr DefaultFormatter() const noexcept;

    /**
     * Set a default formatter.
     * If a newly added appender does not provide a formatter,
     * the default formatter will be used.
     */
    void SetDefaultFormatter(Formatter::Ptr formatter) noexcept;

    /**
     * @brief Set a default formatter.
     *
     * @exception std::invalid_argument The format pattern is invalid.
     */
    void SetDefaultFormatter(std::string_view pattern);

    std::string_view Name() const noexcept;

    std::size_t Capacity() const noexcept;

    //! Convert the logger configuration into a @p YAML string for storage.
    std::string ToYamlString() const noexcept;

private:
    //! A consumer that continuously pops events from the queue and log them.
    void AsyncLogProc() noexcept;

    void SyncLog(const Event& event) noexcept;

    mutable std::mutex mtx_;
    bool async_;
    std::unique_ptr<std::thread> writer_thread_;

    std::string name_;
    std::size_t capacity_;
    log::Level level_;
    std::list<Appender::Ptr> appenders_;
    std::unique_ptr<BlockDeque<Event::Ptr>> event_deque_;
    Formatter::Ptr formatter_ {Formatter::Default()};
};

void Log(Logger::Ptr logger, Event::Ptr event) noexcept;

Logger::Ptr operator<<(Logger::Ptr logger, Event::Ptr event) noexcept;

/**
 * @brief
 * The automatic event writer.
 * When it is destroyed, it will write an event into a logger.
 *
 * @details
 * It can be used as follows:
 * @code {.cpp}
 * EventWriter {logger, event}.MessageStream() << "A message";
 * @endcode
 */
class EventWriter {
public:
    explicit EventWriter(Logger& logger, Event::Ptr event) noexcept;

    EventWriter(const EventWriter&) = delete;

    EventWriter(EventWriter&&) = delete;

    EventWriter& operator=(const EventWriter&) = delete;

    EventWriter& operator=(EventWriter&&) = delete;

    ~EventWriter() noexcept;

    std::ostringstream& MessageStream() noexcept;

private:
    Logger& logger_;
    Event::Ptr event_;
};

/**
 * The logger manager, maintaining a collection of loggers.
 */
class Manager {
public:
    /**
     * @brief Initialize the configuration management for loggers.
     *
     * @warning This method must be called before loading local configurations.
     */
    static void InitConfig() noexcept;

    using Ptr = std::shared_ptr<Manager>;

    explicit Manager(std::string_view name) noexcept;

    Manager(const Manager&) = delete;

    Manager(Manager&&) = delete;

    Manager& operator=(const Manager&) = delete;

    Manager& operator=(Manager&&) = delete;

    std::string_view Name() const noexcept;

    //! Get a logger by its name, creating it if it does not exist.
    Logger::Ptr FindLogger(
        std::string_view name, log::Level level = Level::Info,
        std::optional<std::size_t> capacity = std::nullopt) noexcept;

    void RemoveLogger(std::string_view name) noexcept;

    //! Convert the manager configuration into a @p YAML string for storage.
    std::string ToYamlString() const noexcept;

private:
    mutable std::mutex mtx_;
    std::string name_;
    std::unordered_map<std::string, Logger::Ptr> loggers_;
};

Manager::Ptr RootManager() noexcept;

Logger::Ptr RootLogger() noexcept;

Logger::Ptr FindLogger(std::string_view name) noexcept;

}  // namespace ws::log