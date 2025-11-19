#include "http.h"
#include "io.h"
#include "request.h"
#include "response.h"

#include <cassert>
#include <filesystem>
#include <sstream>


namespace ws::http {

namespace {

std::optional<Parameters> ExtractUserMessage(const Request& request) noexcept {
    static constexpr std::string_view user_tag {"user"};
    static constexpr std::string_view msg_tag {"msg"};

    const auto user {request.Post(user_tag).value_or("")};
    const auto msg {request.Post(msg_tag).value_or("")};
    if (!user.empty() && !user.empty()) {
        return Parameters {{user_tag.data(), user.data()},
                           {msg_tag.data(), msg.data()}};
    } else {
        return std::nullopt;
    }
}

}  // namespace

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

std::filesystem::path ConnectionImpl::root_dir_;

void ConnectionImpl::SetRootDirectory(std::filesystem::path dir) noexcept {
    root_dir_ = std::move(dir);
}

std::filesystem::path ConnectionImpl::GetRootDirectory() noexcept {
    return root_dir_;
}

ConnectionImpl::ConnectionImpl(const FileDescriptor socket) noexcept :
    socket_ {socket} {
    assert(IsValidFileDescriptor(socket_));
}

ConnectionImpl::~ConnectionImpl() noexcept {
    Close();
}

void ConnectionImpl::Close() noexcept {
    if (IsValidFileDescriptor(socket_)) {
        close(socket_);
        socket_ = invalid_file_descriptor;
    }
}

FileDescriptor ConnectionImpl::Socket() const noexcept {
    return socket_;
}

bool ConnectionImpl::KeepAlive() const noexcept {
    return keep_alive_;
}

bool ConnectionImpl::Valid() const noexcept {
    return IsValidFileDescriptor(socket_);
}

std::size_t ConnectionImpl::Receive() {
    io::FileDescriptor io {socket_, socket_};
    std::size_t size {0};

    try {
        while (true) {
            size += read_buf_.ReadFrom(io);
        }
    } catch (const std::system_error& err) {
        if (err.code() != std::errc::resource_unavailable_try_again) {
            throw;
        }
    }

    return size;
}

std::size_t ConnectionImpl::Send() {
    io::FileDescriptor io {socket_, socket_};
    std::size_t header_size {0};
    while (!write_buf_.Empty()) {
        header_size += write_buf_.WriteTo(io);
    }

    std::size_t file_size {0};
    while (file_size < file_.Size()) {
        if (const auto size {write(socket_, file_.Data() + file_size,
                                   file_.Size() - file_size)};
            size >= 0) {
            file_size += size;
        } else {
            ThrowLastSystemError();
        }
    }

    return header_size + file_size;
}

bool ConnectionImpl::Process() noexcept {
    static constexpr std::string_view index_page {"/index.html"};
    static constexpr std::string_view hide_msg_tag {"hide-msg"};

    file_.Unmap();
    if (read_buf_.ReadableSize() == 0) {
        return false;
    }

    Request request;
    Response response {root_dir_};
    std::optional<std::string> error_msg;

    try {
        request.Parse(read_buf_);
    } catch (const std::exception& err) {
        error_msg = err.what();
    }

    keep_alive_ = request.KeepAlive();
    response.SetKeepAlive(keep_alive_);
    if (!error_msg.has_value()) {
        std::string path {request.Path()};
        if (path.empty() || path == "/") {
            path = index_page;
        }

        StatusCode status_code {StatusCode::OK};
        if (path == index_page) {
            auto params {ExtractUserMessage(request).value_or(Parameters {})};
            params.insert({hide_msg_tag.data(), params.empty()
                                                    ? true_tag.data()
                                                    : false_tag.data()});
            response.Build(write_buf_, index_page, params, status_code);
        } else {
            if (auto file {
                    response.Build(write_buf_, std::move(path), status_code)};
                file.has_value()) {
                file_ = std::move(*file);
            }
        }
    } else {
        response.Build(write_buf_, StatusCode::BadRequest, *error_msg);
    }

    return true;
}

}  // namespace ws::http