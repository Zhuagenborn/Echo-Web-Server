#include "field.h"

#include <ctime>
#include <functional>
#include <stdexcept>
#include <unordered_map>


namespace ws::log::field {

void Message::Format(std::ostream& out, const Logger& logger,
                     const Event& event) noexcept {
    out << event.Message();
}

std::string_view Message::Tag() const noexcept {
    return tag;
}

void Level::Format(std::ostream& out, const Logger& logger,
                   const Event& event) noexcept {
    out << LevelToString(event.Level());
}

std::string_view Level::Tag() const noexcept {
    return tag;
}

void ThreadId::Format(std::ostream& out, const Logger& logger,
                      const Event& event) noexcept {
    out << event.ThreadId();
}

std::string_view ThreadId::Tag() const noexcept {
    return tag;
}

DateTime::DateTime(const std::string_view format) noexcept : format_ {format} {
    if (format_.empty()) {
        format_ = default_format;
    }
}

std::string_view DateTime::Tag() const noexcept {
    return tag;
}

void DateTime::Format(std::ostream& out, const Logger& logger,
                      const Event& event) noexcept {
    const auto time {Event::Clock::to_time_t(event.Time())};
    char time_str[0x60] {0};
    std::strftime(time_str, sizeof(time_str), format_.c_str(),
                  std::localtime(&time));
    out << time_str;
}

void FileName::Format(std::ostream& out, const Logger& logger,
                      const Event& event) noexcept {
    out << event.FileName();
}

std::string_view FileName::Tag() const noexcept {
    return tag;
}

void LineNum::Format(std::ostream& out, const Logger& logger,
                     const Event& event) noexcept {
    out << event.LineNum();
}

std::string_view LineNum::Tag() const noexcept {
    return tag;
}

void NewLine::Format(std::ostream& out, const Logger& logger,
                     const Event& event) noexcept {
    out << std::endl;
}

std::string_view NewLine::Tag() const noexcept {
    return tag;
}

void Tab::Format(std::ostream& out, const Logger& logger,
                 const Event& event) noexcept {
    out << "\t";
}

std::string_view Tab::Tag() const noexcept {
    return tag;
}

RawString::RawString(const std::string_view content) noexcept :
    content_ {content} {}

void RawString::Format(std::ostream& out, const Logger& logger,
                       const Event& event) noexcept {
    out << content_;
}

std::string_view RawString::Tag() const noexcept {
    return "";
}

void LoggerName::Format(std::ostream& out, const Logger& logger,
                        const Event& event) noexcept {
    out << logger.Name();
}

std::string_view LoggerName::Tag() const noexcept {
    return tag;
}

bool operator==(const RawField& lhs, const RawField& rhs) noexcept {
    return lhs.raw_str == rhs.raw_str && lhs.content == rhs.content
           && lhs.format == rhs.format;
}

bool operator!=(const RawField& lhs, const RawField& rhs) noexcept {
    return !(lhs == rhs);
}

std::list<RawField> ParsePattern(const std::string_view pattern) {
    std::string raw_str;
    std::list<RawField> raw_fields;
    for (auto i {0}; i < pattern.size(); ++i) {
        if (pattern[i] != '%'
            || (i + 1 < pattern.size() && pattern[i + 1] == '%')) {
            raw_str.append(1, pattern[i]);
            continue;
        } else if (!raw_str.empty()) {
            raw_fields.push_back({true, raw_str, ""});
            raw_str.clear();
        }

        auto j {i + 1}, fmt_begin {0};
        std::string type, fmt;
        auto processing {false};
        while (j < pattern.size()) {
            if (!processing && !std::isalpha(pattern[j]) && pattern[j] != '{'
                && pattern[j] != '}') {
                type = pattern.substr(i + 1, j - i - 1);
                break;
            }

            if (!processing && pattern[j] == '{') {
                type = pattern.substr(i + 1, j - i - 1);
                processing = true;
                fmt_begin = j;
                ++j;
            } else if (processing && pattern[j] == '}') {
                fmt = pattern.substr(fmt_begin + 1, j - fmt_begin - 1);
                processing = false;
                ++j;
                break;
            } else {
                ++j;
                if (j == pattern.size() && type.empty()) {
                    type = pattern.substr(i + 1);
                }
            }
        }

        if (!processing) {
            raw_fields.push_back({false, type, fmt});
        } else {
            throw std::invalid_argument {fmt::format(
                "Invalid log format pattern: '{}'", pattern.substr(i))};
        }

        i = j - 1;
    }

    if (!raw_str.empty()) {
        raw_fields.push_back({true, raw_str, ""});
    }

    return raw_fields;
}

std::list<Formatter::Field::Ptr> RawFieldsToFormatFields(
    const std::list<RawField>& raw_fields) {
    using Creator = std::function<Formatter::Field::Ptr(std::string_view)>;
    static const std::unordered_map<std::string_view, Creator>
        supported_fields {
            {Message::tag,
             [](const std::string_view format) noexcept {
                 return std::make_shared<Message>(format);
             }},
            {Level::tag,
             [](const std::string_view format) noexcept {
                 return std::make_shared<Level>(format);
             }},
            {ThreadId::tag,
             [](const std::string_view format) noexcept {
                 return std::make_shared<ThreadId>(format);
             }},
            {NewLine::tag,
             [](const std::string_view format) noexcept {
                 return std::make_shared<NewLine>(format);
             }},
            {LoggerName::tag,
             [](const std::string_view format) noexcept {
                 return std::make_shared<LoggerName>(format);
             }},
            {DateTime::tag,
             [](const std::string_view format) noexcept {
                 return std::make_shared<DateTime>(format);
             }},
            {FileName::tag,
             [](const std::string_view format) noexcept {
                 return std::make_shared<FileName>(format);
             }},
            {LineNum::tag,
             [](const std::string_view format) noexcept {
                 return std::make_shared<LineNum>(format);
             }},
            {Tab::tag, [](const std::string_view format) noexcept {
                 return std::make_shared<Tab>(format);
             }}};

    std::list<Formatter::Field::Ptr> fields;
    for (const auto& raw_field : raw_fields) {
        if (raw_field.raw_str) {
            fields.push_back(
                std::make_shared<field::RawString>(raw_field.content));
        } else {
            if (const auto field {supported_fields.find(raw_field.content)};
                field != supported_fields.cend()) {
                fields.push_back(field->second(raw_field.format));
            } else {
                throw std::invalid_argument {fmt::format(
                    "Invalid log format field: '{}'", raw_field.content)};
            }
        }
    }

    return fields;
}

}  // namespace ws::log::field