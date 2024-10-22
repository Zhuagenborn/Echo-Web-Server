/**
 * @file http.h
 * @brief The HTTP connection.
 *
 * @author Zhenshuo Chen (chenzs108@outlook.com)
 * @author Liu Guowen (liu.guowen@outlook.com)
 * @par GitHub
 * https://github.com/Zhuagenborn
 * @version 1.0
 * @date 2022-06-10
 */

#pragma once

#include "containers/buffer.h"
#include "ip.h"
#include "util.h"

#include <filesystem>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>


namespace ws::http {

//! HTTP version: 1.1
inline constexpr std::string_view version {"1.1"};

//! HTTP parameters consisting of key-value pairs.
using Parameters = std::unordered_map<std::string, std::string>;

//! HTTP status codes.
enum class StatusCode : std::uint32_t {
    OK = 200,
    BadRequest = 400,
    Forbidden = 403,
    NotFound = 404
};

//! Convert an HTTP status code into a message.
std::string_view StatusCodeToMessage(StatusCode code) noexcept;

//! Convert an HTTP status code into an integer.
std::uint32_t StatusCodeToInteger(StatusCode code) noexcept;

std::ostream& operator<<(std::ostream& os, StatusCode code) noexcept;

//! HTTP methods.
enum class Method { Get, Post, Put, Patch, Delete };

//! Convert an HTTP method into a string.
std::string_view MethodToString(Method method) noexcept;

std::string to_string(Method method) noexcept;

/**
 * @brief Convert a string into an HTTP method.
 *
 * @exception std::invalid_argument The string does not represent an HTTP method.
 */
Method StringToMethod(std::string str);

std::ostream& operator<<(std::ostream& os, Method method) noexcept;

//! HTTP uses @p CRLF as the line separator.
inline constexpr std::string_view new_line {"\r\n"};

/**
 * @brief
 * Get a file's content type.
 * @p application/octet-stream is used to indicate that a file contains arbitrary binary data.
 */
std::string_view ContentTypeByFileName(std::string_view name) noexcept;

/**
 * @brief
 * Decode an URL-encoded character.
 * They start with a @p %, followed by a pair of hexadecimal digits.
 *
 * @exception std::invalid_argument The string does not represent an URL-encoded character.
 */
char DecodeURLEncodedCharacter(const std::string& str);

/**
 * @brief
 * Decode an URL-encoded string.
 * URL-encoded characters start with a @p %, followed by a pair of hexadecimal digits.
 *
 * @exception std::invalid_argument The string contains invalid URL-encoded characters.
 */
std::string DecodeURLEncodedString(const std::string& str);

/**
 * @brief
 * Get the HTML placeholder for an HTTP parameter.
 * It can be used to insert actual data into an HTML page.
 */
std::string HTMLPlaceholder(std::string_view key) noexcept;

/**
 * @brief
 * Put HTTP parameters into an HTML template in place of the corresponding placeholders.
 *
 * @warning
 * This method will ignore parameters that do not have a corresponding placeholder in the template.
 */
std::string PutParamIntoHTML(std::string html, const Parameters& params);

/**
 * @brief The core implementation of HTTP connection.
 *
 * @details
 * A valid HTTP request must use @p POST method and contain two variables:
 * - @p user: A user name.
 * - @p msg: A message.
 *
 * @warning
 * This class is an internal implementation class.
 * Developers should use @p Connection instead of this.
 */
class ConnectionImpl {
public:
    using Ptr = std::shared_ptr<ConnectionImpl>;

    //! Set the root directory.
    static void SetRootDirectory(std::filesystem::path dir) noexcept;

    //! Get the root directory.
    static std::filesystem::path GetRootDirectory() noexcept;

    ConnectionImpl(const ConnectionImpl&) = delete;

    ConnectionImpl(ConnectionImpl&&) = delete;

    ConnectionImpl& operator=(const ConnectionImpl&) = delete;

    ConnectionImpl& operator=(ConnectionImpl&&) = delete;

    //! Close the connection.
    void Close() noexcept;

    //! Whether the connection is valid.
    bool Valid() const noexcept;

    //! Get the socket.
    FileDescriptor Socket() const noexcept;

    //! Receive an HTTP request.
    std::size_t Receive();

    //! Send an HTTP response.
    std::size_t Send();

    //! Whether the connection keeps alive.
    bool KeepAlive() const noexcept;

    /**
     * @brief Process the HTTP request.
     *
     * @details
     * For the first request that does not contain a user's input,
     * It will reply with a form for user input.
     * Otherwise, it will reply both a user's previous input and a form for new input.
     *
     * @return @p false if the reading buffer for request is empty, otherwise @p true.
     */
    bool Process() noexcept;

protected:
    static constexpr std::string_view true_tag {"true"};

    static constexpr std::string_view false_tag {"false"};

    explicit ConnectionImpl(FileDescriptor socket) noexcept;

    virtual ~ConnectionImpl() noexcept;

    static std::filesystem::path root_dir_;

    FileDescriptor socket_ {invalid_file_descriptor};
    bool keep_alive_ {false};

    IOBuffer read_buf_;
    IOBuffer write_buf_;

    //! The requested file.
    MappedReadOnlyFile file_;
};

//! The HTTP connection.
template <ValidIPAddr IPAddr>
class Connection : public ConnectionImpl {
public:
    using Ptr = std::shared_ptr<Connection>;

    explicit Connection(const FileDescriptor socket, IPAddr addr) noexcept :
        ConnectionImpl {socket}, addr_ {std::move(addr)} {}

    std::string IPAddress() const noexcept {
        return addr_.IPAddress();
    }

    std::uint16_t Port() const noexcept {
        return addr_.Port();
    }

private:
    IPAddr addr_;
};

}  // namespace ws::http