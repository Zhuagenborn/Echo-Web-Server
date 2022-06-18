/**
 * @file request.h
 * @brief The HTTP request parser.
 *
 * @author Zhenshuo Chen (chenzs108@outlook.com)
 * @author Liu Guowen (liu.guowen@outlook.com)
 * @par GitHub
 * https://github.com/Zhuagenborn
 * @version 1.0
 * @date 2022-06-14
 */

#pragma once

#include "containers/buffer.h"
#include "http.h"

#include <memory>
#include <optional>


namespace ws::http {

/**
 * The HTTP request parser.
 */
class Request {
public:
    class State;

    Request() noexcept;

    /**
     * @brief Create a parser and parse an HTTP request.
     *
     * @exception std::invalid_argument The HTTP request is invalid.
     */
    explicit Request(Buffer& buf);

    ~Request() noexcept;

    /**
     * @brief Parse an HTTP request.
     *
     * @exception std::invalid_argument The HTTP request is invalid.
     */
    void Parse(Buffer& buf);

    /**
     * @brief Get an HTTP header by its key.
     *
     * @warning The query is case-sensitive.
     */
    std::optional<std::string_view> Header(std::string_view key) const noexcept;

    /**
     * @brief Get an HTTP @p POST variable by its key.
     *
     * @warning The query is case-sensitive.
     */
    std::optional<std::string_view> Post(std::string_view key) const noexcept;

    //! Get the number of HTTP @p POST variables.
    std::size_t PostSize() const noexcept;

    //! Get the HTTP method.
    http::Method Method() const noexcept;

    //! Get the path.
    std::string_view Path() const noexcept;

    //! Get the HTTP version.
    std::string_view Version() const noexcept;

    //! Whether the request keeps alive.
    bool KeepAlive() const noexcept;

private:
    friend class NotStarted;
    friend class Header;
    friend class Body;

    //! Set a new parsing state.
    void SetState(std::unique_ptr<State> state) noexcept;

    //! Clear the cache.
    void Clear() noexcept;

    std::unique_ptr<State> state_;

    http::Method method_ {Method::Get};
    std::string version_;
    std::string path_;

    Parameters headers_;
    Parameters post_;
};

}  // namespace ws::http