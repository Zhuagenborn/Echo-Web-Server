/**
 * @file field.h
 * @brief Sub-formatters for different event fields.
 *
 * @author Chen Zhenshuo (chenzs108@outlook.com)
 * @par GitHub
 * https://github.com/Zhuagenborn
 * @version 1.0
 * @date 2022-05-15
 *
 * @example src/log/field_test.cpp
 */

#pragma once

#include "log.h"

#include <list>
#include <string>
#include <string_view>


namespace ws::log::field {

class Message : public Formatter::Field {
public:
    static constexpr std::string_view tag {"m"};

    using Field::Field;

    void Format(std::ostream& out, const Logger& logger,
                const Event& event) noexcept override;

    std::string_view Tag() const noexcept override;
};

class Level : public Formatter::Field {
public:
    static constexpr std::string_view tag {"p"};

    using Field::Field;

    void Format(std::ostream& out, const Logger& logger,
                const Event& event) noexcept override;

    std::string_view Tag() const noexcept override;
};

class ThreadId : public Formatter::Field {
public:
    static constexpr std::string_view tag {"t"};

    using Field::Field;

    void Format(std::ostream& out, const Logger& logger,
                const Event& event) noexcept override;

    std::string_view Tag() const noexcept override;
};

class DateTime : public Formatter::Field {
public:
    static constexpr std::string_view tag {"d"};

    static constexpr std::string_view default_format {"%Y-%m-%d %H:%M:%S"};

    DateTime(std::string_view format = default_format) noexcept;

    void Format(std::ostream& out, const Logger& logger,
                const Event& event) noexcept override;

    std::string_view Tag() const noexcept override;

private:
    std::string format_;
};

class FileName : public Formatter::Field {
public:
    static constexpr std::string_view tag {"f"};

    using Field::Field;

    void Format(std::ostream& out, const Logger& logger,
                const Event& event) noexcept override;

    std::string_view Tag() const noexcept override;
};

class LineNum : public Formatter::Field {
public:
    static constexpr std::string_view tag {"l"};

    using Field::Field;

    void Format(std::ostream& out, const Logger& logger,
                const Event& event) noexcept override;

    std::string_view Tag() const noexcept override;
};

class NewLine : public Formatter::Field {
public:
    static constexpr std::string_view tag {"n"};

    using Field::Field;

    void Format(std::ostream& out, const Logger& logger,
                const Event& event) noexcept override;

    std::string_view Tag() const noexcept override;
};

class Tab : public Formatter::Field {
public:
    static constexpr std::string_view tag {"T"};

    using Field::Field;

    void Format(std::ostream& out, const Logger& logger,
                const Event& event) noexcept override;

    std::string_view Tag() const noexcept override;
};

class RawString : public Formatter::Field {
public:
    RawString(std::string_view content) noexcept;

    void Format(std::ostream& out, const Logger& logger,
                const Event& event) noexcept override;

    std::string_view Tag() const noexcept override;

private:
    std::string content_;
};

class LoggerName : public Formatter::Field {
public:
    static constexpr std::string_view tag {"c"};

    using Field::Field;

    void Format(std::ostream& out, const Logger& logger,
                const Event& event) noexcept override;

    std::string_view Tag() const noexcept override;
};

/**
 * @brief The raw field.
 *
 * @details
 * It is an internal structure for conversion between field tags and sub-formatters.
 */
struct RawField {
    //! Whether the raw field is a raw string.
    bool raw_str;

    /**
     * @brief A raw string.
     *
     * @details
     * If the raw field is a raw string, this field will save its content.
     * Otherwise this field is a tag. For example, @p t means a thread ID.
     */
    std::string content;

    /**
     * @brief An optional format.
     *
     * @details
     * For example, @p %Y-%m-%d can be used for date display.
     * If the raw field is a raw string, this field will be empty.
     */
    std::string format;
};

bool operator==(const RawField&, const RawField&) noexcept;

bool operator!=(const RawField&, const RawField&) noexcept;

//! Parse a pattern string into raw fields.
std::list<RawField> ParsePattern(std::string_view pattern);

//! Convert raw fields into sub-formatters.
std::list<Formatter::Field::Ptr> RawFieldsToFormatFields(
    const std::list<RawField>& raw_fields);

}  // namespace ws::log::field