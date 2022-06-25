/**
 * @file response.h
 * @brief The HTTP response.
 *
 * @author Zhenshuo Chen (chenzs108@outlook.com)
 * @author Liu Guowen (liu.guowen@outlook.com)
 * @par GitHub
 * https://github.com/Zhuagenborn
 * @version 1.0
 * @date 2022-06-09
 */

#pragma once

#include "containers/buffer.h"
#include "http.h"
#include "util.h"

#include <filesystem>
#include <optional>


namespace ws::http {

/**
 * The HTTP response builder.
 */
class Response {
public:
    /**
     * @brief Create an HTTP response builder.
     *
     * @param root_dir
     * A root directory.
     * If using a relative path to access a file when making response,
     * that path will be relative to the root directory.
     */
    explicit Response(std::filesystem::path root_dir) noexcept;

    ~Response() noexcept;

    Response(const Response&) = delete;

    Response(Response&&) = delete;

    Response& operator=(const Response&) = delete;

    Response& operator=(Response&&) = delete;

    //! Whether the connection should keep alive.
    Response& SetKeepAlive(bool set) noexcept;

    /**
     * @brief Build an HTTP response from a file request.
     *
     * @param[out] buf  An output buffer where the response content will be written to.
     * @param file      A file path. If it is a relative path, it will be relative to the root directory.
     * @param[out] code The HTTP status code representing the file request.
     * @return
     * A read-only mapped file.  @p std::nullopt if the file request failed.
     * If this method returns a valid mapped file,
     * developers should send file content after sending the response header in the buffer.
     */
    std::optional<MappedReadOnlyFile> Build(Buffer& buf,
                                            std::filesystem::path file,
                                            StatusCode& code) noexcept;

    /**
     * @brief Build an HTTP response from a file request.
     *
     * @param[out] buf  An output buffer where the response content will be written to.
     * @param html
     * The path of an HTML page.
     * If it is a relative path, it will be relative to the root directory.
     * @param params
     * HTTP parameters.
     * If @p html contains placeholders, those parameters can be used for replacement.
     * @param[out] code The HTTP status code representing the file request.
     *
     * @warning
     * - This method does not check whether all placeholders in an HTML page have been replaced with parameters.
     * - The parameter name is case-sensitive.
     */
    void Build(Buffer& buf, std::filesystem::path html,
               const Parameters& params, StatusCode& code) noexcept;

    /**
     * @brief Build a response from HTTP status.
     *
     * @param[out] buf  An output buffer where the response content will be written to.
     * @param code      An HTTP status code.
     * @param msg       An optional message.
     */
    void Build(Buffer& buf, StatusCode code, std::string msg = "") noexcept;

private:
    //! Clear settings.
    void Clear() noexcept;

    //! Build an HTTP response from the current settings.
    void Build(Buffer& buf, const Parameters* params = nullptr) noexcept;

    //! Check the file status if there is a file request.
    void CheckFile();

    //! Map the file into memory if there is a valid file request.
    void MapFile();

    //! Add an HTTP status line.
    void AddStatusLine(Buffer& buf) const noexcept;

    //! Add HTTP headers that are not relevant to the content of the response.
    void AddHeaders(Buffer& buf) const noexcept;

    /**
     * @brief Add HTTP headers that are relevant to the mapped read-only file.
     *
     * @note
     * This method does not append file content to the buffer,
     * because the base address of file content will be returned to developers.
     */
    void AddMappedContent(Buffer& buf) noexcept;

    //! Add HTTP headers and generated HTML content from parameters.
    void AddParamContent(Buffer& buf, const Parameters& params) const noexcept;

    //! Add HTTP headers and a predefined error-handling HTML page.
    void AddPredefinedErrorContent(Buffer& buf,
                                   std::string_view msg = "") noexcept;

    std::filesystem::path root_dir_;
    std::filesystem::path file_path_;

    MappedReadOnlyFile file_;

    bool keep_alive_ {false};
    StatusCode status_code_ {StatusCode::OK};
};

}  // namespace ws::http