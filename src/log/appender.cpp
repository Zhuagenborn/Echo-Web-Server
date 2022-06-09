#include "log.h"

#include <stdexcept>
#include <syncstream>


namespace ws::log {

std::string_view AppenderTypeToString(const AppenderType type) noexcept {
    static const std::unordered_map<AppenderType, std::string_view> types {
        {AppenderType::StdOut, "StdOut"}, {AppenderType::File, "File"}};

    return types.at(type);
}

std::string to_string(const AppenderType type) noexcept {
    return AppenderTypeToString(type).data();
}

AppenderType StringToAppenderType(std::string str) {
    static const std::unordered_map<std::string_view, AppenderType> types {
        {"STDOUT", AppenderType::StdOut}, {"FILE", AppenderType::File}};

    str = StringToUpper(str);
    if (const auto type {types.find(str)}; type != types.cend()) {
        return type->second;
    } else {
        throw std::invalid_argument {
            fmt::format("Invalid log appender type: '{}'", str)};
    }
}

Appender::Appender(const std::string_view pattern) {
    SetFormatter(pattern);
}

Appender::Appender(const Formatter::Ptr formatter) noexcept :
    formatter_ {formatter} {}

Formatter::Ptr Appender::Formatter() const noexcept {
    const std::lock_guard locker {mtx_};
    return formatter_;
}

void Appender::SetFormatter(const Formatter::Ptr formatter) noexcept {
    const std::lock_guard locker {mtx_};
    formatter_ = formatter;
}

void Appender::SetFormatter(const std::string_view pattern) {
    SetFormatter(std::make_shared<log::Formatter>(pattern));
}

void StdOutAppender::Log(const Logger& logger, const Event& event) noexcept {
    const std::lock_guard locker {mtx_};
    std::osyncstream {std::cout} << formatter_->Format(logger, event)
                                 << std::flush;
}

std::string StdOutAppender::ToYamlString() const noexcept {
    const std::lock_guard locker {mtx_};

    YAML::Node node;
    node["type"] = AppenderTypeToString(AppenderType::StdOut).data();
    if (formatter_) {
        node["formatter"] = formatter_->Pattern().data();
    }

    std::ostringstream ss;
    ss << node;
    return ss.str();
}

FileAppender::FileAppender(const std::string_view file_name,
                           const Formatter::Ptr formatter) :
    file_name_ {file_name}, Appender {formatter} {
    file_.open(file_name_, std::ofstream::app);
    if (!file_.good()) {
        ThrowLastSystemError();
    }
}

std::string FileAppender::ToYamlString() const noexcept {
    const std::lock_guard locker {mtx_};

    YAML::Node node;
    node["type"] = AppenderTypeToString(AppenderType::StdOut).data();
    node["file"] = file_name_;
    if (formatter_) {
        node["formatter"] = formatter_->Pattern().data();
    }

    std::ostringstream ss;
    ss << node;
    return ss.str();
}

void FileAppender::Log(const Logger& logger, const Event& event) noexcept {
    const std::lock_guard locker {mtx_};
    file_ << formatter_->Format(logger, event) << std::flush;
}

}  // namespace ws::log