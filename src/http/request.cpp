#include "request.h"
#include "util.h"

#include <cassert>
#include <regex>
#include <stdexcept>


namespace ws::http {

//! The interface of parsing state.
class Request::State {
public:
    explicit State(Request& parser) noexcept : parser_ {parser} {}

    virtual ~State() noexcept = default;

    virtual void Parse(const std::string& content) = 0;

protected:
    Request& parser_;
};

//! The parsing has not started yet.
class NotStarted : public Request::State {
public:
    using State::State;

    /**
     * @brief Parse the HTTP status line.
     *
     * @exception std::invalid_argument The HTTP status line is invalid.
     */
    void Parse(const std::string& line) override {
        ParseStatusLine(line);
    }

private:
    void ParseStatusLine(const std::string& line);
};

//! The parser is processing HTTP headers.
class Header : public Request::State {
public:
    using State::State;

    /**
     * @brief Parse an HTTP header.
     *
     * @exception std::invalid_argument No empty line between HTTP headers and the body.
     */
    void Parse(const std::string& line) override;
};

//! The parser is processing the HTTP body.
class Body : public Request::State {
public:
    using State::State;

    /**
     * @brief Parse the HTTP body.
     *
     * @exception std::invalid_argument Unsupported HTTP method.
     */
    void Parse(const std::string& body) override;

private:
    /**
     * @brief Parse an HTTP @p POST body.
     *
     * @exception std::invalid_argument Unsupported HTTP content type.
     */
    void ParsePost(const std::string& body);

    /**
     * @brief
     * Parse an HTTP @p POST body with content type @p application/x-www-form-urlencoded.
     *
     * @exception std::invalid_argument Invalid HTTP @p POST data.
     */
    void ParseURLEncodedPost(const std::string& body);
};

//! The parsing has finished.
class Finished : public Request::State {
public:
    using State::State;

    //! Throw a @p std::invalid_argument exception because the parsing shoud have finished.
    [[noreturn]] void Parse(const std::string& content) override {
        throw std::invalid_argument {"The parsing shoud have finished"};
    }
};

Request::Request() noexcept = default;

Request::~Request() noexcept = default;

Request::Request(Buffer& buf) {
    Parse(buf);
}

void Request::Parse(Buffer& buf) {
    Clear();
    if (buf.Empty()) {
        throw std::invalid_argument {"The buffer is empty"};
    }

    auto content {buf.ReadableString()};
    while (!content.empty()) {
        const auto line_end {content.find(new_line)};
        const auto line {content.substr(0, line_end)};

        assert(state_);
        state_->Parse(line);

        buf.Retrieve(line.length());
        content = content.substr(line.length());
        if (line_end != std::string::npos) {
            content = content.substr(new_line.length());
            buf.Retrieve(new_line.length());
        }
    }
}

void Request::SetState(std::unique_ptr<State> state) noexcept {
    state_ = std::move(state);
}

void Request::Clear() noexcept {
    SetState(std::make_unique<NotStarted>(*this));
    method_ = Method::Get;
    version_.clear();
    path_.clear();
    headers_.clear();
    post_.clear();
}

std::size_t Request::PostSize() const noexcept {
    return post_.size();
}

bool Request::KeepAlive() const noexcept {
    if (const auto conn {headers_.find("Connection")};
        conn != headers_.cend()) {
        return version_ == "1.1" && conn->second == "keep-alive";
    } else {
        return false;
    }
}

std::string_view Request::Version() const noexcept {
    return version_;
}

std::string_view Request::Path() const noexcept {
    return path_;
}

http::Method Request::Method() const noexcept {
    return method_;
}

std::optional<std::string_view> Request::Post(
    const std::string_view key) const noexcept {
    if (const auto val {post_.find(key.data())}; val != post_.cend()) {
        return val->second;
    } else {
        return std::nullopt;
    }
}

std::optional<std::string_view> Request::Header(
    const std::string_view key) const noexcept {
    if (const auto val {headers_.find(key.data())}; val != headers_.cend()) {
        return val->second;
    } else {
        return std::nullopt;
    }
}

void NotStarted::ParseStatusLine(const std::string& line) {
    const std::regex pattern {"^([^ ]*) ([^ ]*) HTTP/([^ ]*)$"};
    std::smatch matches;
    if (std::regex_match(line, matches, pattern)) {
        parser_.method_ = StringToMethod(matches[1]);
        parser_.path_ = matches[2];
        parser_.version_ = matches[3];
        parser_.SetState(std::make_unique<Header>(parser_));
    } else {
        throw std::invalid_argument {
            fmt::format("Invalid HTTP status line: '{}'", line)};
    }
}

void Header::Parse(const std::string& line) {
    const std::regex pattern {"^([^:]*): ?(.*)$"};
    std::smatch matches;
    if (std::regex_match(line, matches, pattern)) {
        parser_.headers_.emplace(matches[1], matches[2]);
    } else if (line.empty()) {
        parser_.SetState(std::make_unique<Body>(parser_));
    } else {
        throw std::invalid_argument {
            "There must be an empty line between HTTP headers and the body"};
    }
}

void Body::Parse(const std::string& body) {
    const auto method {parser_.Method()};
    switch (method) {
        case Method::Post: {
            ParsePost(body);
            break;
        }
        default: {
            throw std::invalid_argument {fmt::format(
                "Unsupported HTTP method: '{}'", to_string(method))};
        }
    }

    parser_.SetState(std::make_unique<Finished>(parser_));
}

void Body::ParsePost(const std::string& body) {
    if (const auto content_type {parser_.Header("Content-Type").value_or("")};
        content_type == "application/x-www-form-urlencoded") {
        ParseURLEncodedPost(body);
    } else {
        throw std::invalid_argument {
            fmt::format("Unsupported HTTP content type: '{}'", content_type)};
    }
}

void Body::ParseURLEncodedPost(const std::string& body) {
    auto& post {parser_.post_};
    auto decoded_body {DecodeURLEncodedString(body)};
    std::string_view view {decoded_body};
    std::string key, val;
    std::size_t begin {0}, end {0};
    for (; end < view.size(); ++end) {
        switch (view[end]) {
            case '=': {
                key = view.substr(begin, end - begin);
                begin = end + 1;
                break;
            }
            case '+': {
                decoded_body[end] = ' ';
                break;
            }
            case '&': {
                val = view.substr(begin, end - begin);
                begin = end + 1;
                if (!key.empty()) {
                    post.emplace(std::move(key), std::move(val));
                } else {
                    throw std::invalid_argument {
                        fmt::format("Invalid HTTP POST data: '{}'", body)};
                }

                break;
            }
            default: {
                break;
            }
        }
    }

    // Save the last key-value pair.
    assert(begin <= end);
    val = view.substr(begin, end - begin);
    if (!key.empty() && !post.contains(key) && !val.empty()) {
        post.emplace(std::move(key), std::move(val));
    } else {
        throw std::invalid_argument {
            fmt::format("Invalid HTTP POST data: '{}'", body)};
    }
}

}  // namespace ws::http