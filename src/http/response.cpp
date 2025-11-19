#include "response.h"

#include <cassert>
#include <sstream>


namespace ws::http {

Response::Response(std::filesystem::path root_dir) noexcept :
    root_dir_ {std::move(root_dir)} {}

Response::~Response() noexcept {
    Clear();
}

void Response::Clear() noexcept {
    file_.Unmap();
    status_code_ = StatusCode::OK;
    file_path_.clear();
}

Response& Response::SetKeepAlive(const bool set) noexcept {
    keep_alive_ = set;
    return *this;
}

std::optional<MappedReadOnlyFile> Response::Build(Buffer& buf,
                                                  std::filesystem::path file,
                                                  StatusCode& code) noexcept {
    Clear();
    file_path_ = std::move(file);
    status_code_ = StatusCode::OK;
    Build(buf);
    code = status_code_;
    return file_.Data() ? std::optional {std::move(file_)} : std::nullopt;
}

void Response::Build(Buffer& buf, std::filesystem::path file,
                     const Parameters& params, StatusCode& code) noexcept {
    Clear();
    file_path_ = std::move(file);
    status_code_ = StatusCode::OK;
    Build(buf, &params);
    code = status_code_;
}

void Response::Build(Buffer& buf, const StatusCode code,
                     std::string msg) noexcept {
    static constexpr std::string_view http_status_page {"/http-status.html"};
    static constexpr std::string_view status_code_tag {"status-code"};
    static constexpr std::string_view status_tag {"status"};
    static constexpr std::string_view msg_tag {"msg"};

    Clear();
    status_code_ = code;
    file_path_ = http_status_page;

    const Parameters params {
        {status_code_tag.data(), std::to_string(StatusCodeToInteger(status_code_))},
        {status_tag.data(), StatusCodeToMessage(status_code_).data()},
        {msg_tag.data(), std::move(msg)}
    };

    Build(buf, &params);
}

void Response::Build(Buffer& buf, const Parameters* const params) noexcept {
    std::optional<std::string> error_msg;

    try {
        if (!root_dir_.empty()) {
            // The form of an HTTP path is "/path/to/file".
            // Using `relative_path` can get its relative path which is "path/to/file".
            // Otherwise `root_dir_ / file_path_` only returns `file_path_`,
            // because `file_path_` is considered as an absolute path.
            file_.Map(root_dir_ / file_path_.relative_path());
        } else {
            file_.Map(file_path_);
        }
    } catch (const std::exception& err) {
        status_code_ = StatusCode::BadRequest;
        error_msg = err.what();
    }

    AddStatusLine(buf);
    AddHeaders(buf);
    if (!error_msg.has_value()) {
        if (params) {
            AddParamContent(buf, *params);
        } else {
            AddMappedContent(buf);
        }
    } else {
        AddPredefinedErrorContent(buf, *error_msg);
    }
}

void Response::AddStatusLine(Buffer& buf) const noexcept {
    buf.Append(
        fmt::format("HTTP/{} {} {}", version, StatusCodeToInteger(status_code_),
                    StatusCodeToMessage(status_code_)),
        NewLine::CRLF);
}

void Response::AddHeaders(Buffer& buf) const noexcept {
    buf.Append("Connection: ");
    if (keep_alive_) {
        buf.Append("keep-alive", NewLine::CRLF);
        buf.Append("keep-alive: max=6, timeout=120", NewLine::CRLF);
    } else {
        buf.Append("close", NewLine::CRLF);
    }
}

void Response::AddMappedContent(Buffer& buf) noexcept {
    assert(file_.Data());

    buf.Append(fmt::format("Content-type: {}",
                           ContentTypeByFileName(file_path_.c_str())),
               NewLine::CRLF);
    buf.Append(fmt::format("Content-length: {}", file_.Size()), NewLine::CRLF);
    buf.Append(new_line);
}

void Response::AddParamContent(Buffer& buf,
                               const Parameters& params) const noexcept {
    assert(file_.Data());

    buf.Append(fmt::format("Content-type: {}",
                           ContentTypeByFileName(file_path_.c_str())),
               NewLine::CRLF);

    std::string content {reinterpret_cast<const char*>(file_.Data()),
                         file_.Size()};
    for (const auto& [key, val] : params) {
        content = ReplaceAllSubstring(content, HTMLPlaceholder(key), val);
    }

    const auto lines {SplitStringToLines(content)};
    std::size_t length {new_line.length() * (lines.size() - 1)};
    std::for_each(lines.cbegin(), lines.cend(),
                  [&length](const auto& line) { length += line.size(); });

    buf.Append(fmt::format("Content-length: {}", length), NewLine::CRLF);
    buf.Append(new_line);

    for (auto i {0}; i != lines.size(); ++i) {
        if (i != lines.size() - 1) {
            buf.Append(lines[i], NewLine::CRLF);
        } else {
            buf.Append(lines[i]);
        }
    }
}

void Response::AddPredefinedErrorContent(Buffer& buf,
                                         const std::string_view msg) noexcept {
    buf.Append("Content-type: text/html", NewLine::CRLF);

    std::ostringstream ss;
    ss << "<html>" << new_line;
    ss << "<title>ERROR</title>" << new_line;
    ss << "<body>" << new_line;
    ss << fmt::format("<p>{} : {}</p>", StatusCodeToInteger(status_code_),
                      StatusCodeToMessage(status_code_))
       << new_line;
    if (!msg.empty()) {
        ss << fmt::format("<p>{}</p>", msg) << new_line;
    }
    ss << "</body>" << new_line;
    ss << "</html>";

    const auto body {ss.str()};
    buf.Append(fmt::format("Content-length: {}", body.size()), NewLine::CRLF);
    buf.Append(new_line);
    buf.Append(body);
}

}  // namespace ws::http