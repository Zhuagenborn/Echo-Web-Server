#include "http.h"
#include "util.h"

#include <filesystem>
#include <sstream>


namespace ws::http {

std::string_view ContentTypeByFileName(const std::string_view name) noexcept {
    static const std::unordered_map<std::string_view, std::string_view> types {
        {".html", "text/html"},
        {".xml", "text/xml"},
        {".xhtml", "application/xhtml+xml"},
        {".txt", "text/plain"},
        {".rtf", "application/rtf"},
        {".pdf", "application/pdf"},
        {".word", "application/nsword"},
        {".png", "image/png"},
        {".gif", "image/gif"},
        {".jpg", "image/jpeg"},
        {".jpeg", "image/jpeg"},
        {".au", "audio/basic"},
        {".mpeg", "video/mpeg"},
        {".mpg", "video/mpeg"},
        {".avi", "video/x-msvideo"},
        {".gz", "application/x-gzip"},
        {".tar", "application/x-tar"},
        {".css", "text/css"},
        {".js", "text/javascript"},
    };

    const auto extension {
        StringToLower(std::filesystem::path {name}.extension())};
    return types.contains(extension.c_str()) ? types.at(extension.c_str())
                                             : "application/octet-stream";
}

std::string_view StatusCodeToMessage(const StatusCode code) noexcept {
    static const std::unordered_map<StatusCode, std::string_view> msgs {
        {StatusCode::OK, "OK"},
        {StatusCode::BadRequest, "Bad Request"},
        {StatusCode::Forbidden, "Forbidden"},
        {StatusCode::NotFound, "Not Found"}};

    return msgs.at(code);
}

std::ostream& operator<<(std::ostream& os, const StatusCode code) noexcept {
    return os << StatusCodeToMessage(code);
}

std::uint32_t StatusCodeToInteger(const StatusCode code) noexcept {
    return static_cast<std::uint32_t>(code);
}

std::string_view MethodToString(const Method method) noexcept {
    static const std::unordered_map<Method, std::string_view> methods {
        {Method::Get, "GET"},
        {Method::Patch, "PATCH"},
        {Method::Post, "POST"},
        {Method::Delete, "DELETE"},
        {Method::Put, "PUT"}};

    return methods.at(method);
}

std::string to_string(const Method method) noexcept {
    return MethodToString(method).data();
}

Method StringToMethod(std::string str) {
    static const std::unordered_map<std::string_view, Method> methods {
        {"GET", Method::Get},
        {"PATCH", Method::Patch},
        {"POST", Method::Post},
        {"DELETE", Method::Delete},
        {"PUT", Method::Put}};

    str = StringToUpper(str);
    if (const auto method {methods.find(str)}; method != methods.cend()) {
        return method->second;
    } else {
        throw std::invalid_argument {
            fmt::format("Invalid HTTP method: '{}'", str)};
    }
}

std::ostream& operator<<(std::ostream& os, const Method method) noexcept {
    return os << MethodToString(method);
}

char DecodeURLEncodedCharacter(const std::string& str) {
    static constexpr std::size_t encoded_length {3};
    if (str.length() == encoded_length && str.front() == '%') {
        const auto ascii {std::stoi(str.substr(1), nullptr, 16)};
        return static_cast<char>(ascii);
    } else {
        throw std::invalid_argument {
            fmt::format("Invalid HTTP URL-encoding character: '{}'", str)};
    }
}

std::string DecodeURLEncodedString(const std::string& str) {
    static constexpr std::size_t encoded_length {3};
    std::ostringstream ss;
    std::size_t i {0};
    while (i < str.size()) {
        if (str[i] == '%') {
            if (const auto encoded {str.substr(i, encoded_length)};
                encoded.length() == encoded_length) {
                ss << DecodeURLEncodedCharacter(encoded);
                i += encoded_length;
            } else {
                throw std::invalid_argument {fmt::format(
                    "Invalid HTTP URL-encoding strings: '{}'", str)};
            }
        } else {
            ss << str[i++];
        }
    }

    return ss.str();
}

std::string HTMLPlaceholder(const std::string_view key) noexcept {
    return fmt::format("<${}$>", key);
}

std::string PutParamIntoHTML(std::string html, const Parameters& params) {
    for (const auto& [key, val] : params) {
        html = ReplaceAllSubstring(html, HTMLPlaceholder(key), val);
    }

    return html;
}

}  // namespace ws::http