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

#include <iostream>
#include <string>
#include <string_view>
#include <unordered_map>


namespace ws::http {

//! HTTP version: 1.1
constexpr std::string_view version {"1.1"};

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
constexpr std::string_view new_line {"\r\n"};

/**
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

}  // namespace ws::http