/**
 * @file buffer.h
 * @brief The auto-expandable buffer, supporting storing bytes and strings.
 *
 * @author Chen Zhenshuo (chenzs108@outlook.com)
 * @author Liu Guowen (liu.guowen@outlook.com)
 * @par GitHub
 * https://github.com/Zhuagenborn
 * @version 1.0
 * @date 2022-05-03
 *
 * @example tests/containers/buffer_test.cpp
 */

#pragma once

#include <atomic>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace ws {

namespace io {

class IReadWriter;

}

//! New-line characters
enum class NewLine {
    //! @p \n
    LF,

    //! @p \r\n
    CRLF
};

/**
 * @brief An auto-expandable buffer, supporting storing bytes and strings.
 *
 * @details
 *                 Writing Offset ──────┐
 * Reading Offset ─────┐                │
 *                     │                │
 *                     │                │
 * ┌───────────────────▼────────────────▼────────────────┐
 * │ Prependable Bytes │ Readable Bytes │ Writable Bytes │
 * └───────────────────┴────────────────┴────────────────┘
 * Prependable space can be reused.
 */
class Buffer {
public:
    //! Create a buffer with an initial size.
    explicit Buffer(std::size_t size = 1000) noexcept;

    //! Create a buffer with bytes.
    explicit Buffer(std::span<const std::byte> bytes) noexcept;

    //! Create a buffer with bytes.
    explicit Buffer(std::initializer_list<std::byte> bytes) noexcept;

    //! Create a buffer with a string.
    explicit Buffer(std::string_view str) noexcept;

    Buffer(const Buffer&) noexcept;

    Buffer(Buffer&&) noexcept;

    Buffer& operator=(const Buffer&) noexcept;

    Buffer& operator=(Buffer&&) noexcept;

    //! Get the current writable size without expanding.
    std::size_t WritableSize() const noexcept;

    //! Get the readable size.
    std::size_t ReadableSize() const noexcept;

    //! Peek the first byte without moving the reading offset.
    std::optional<std::byte> Peek() const noexcept;

    //! Get readable bytes without moving the reading offset.
    std::span<const std::byte> ReadableBytes() const noexcept;

    /**
     * @brief Get a readable string without moving the reading offset.
     *
     * @warning Developers should ensure that the stored bytes are printable.
     */
    std::string ReadableString() const noexcept;

    /**
     * @brief Get writable space for editing.
     *
     * @warning
     * If developers directly write data using the base address of writable space,
     * they must manually adjust the writing offset with @p HasWritten.
     */
    std::span<std::byte> WritableBytes() const noexcept;

    //! Append bytes to the buffer and move forward the writing offset.
    void Append(std::span<const std::byte> bytes) noexcept;

    //! Append bytes to the buffer and move forward the writing offset.
    void Append(std::initializer_list<std::byte> bytes) noexcept;

    //! Append a string and an optional new-line character to the buffer and move forward the writing offset.
    void Append(std::string_view str,
                std::optional<NewLine> new_line = std::nullopt) noexcept;

    //! Append data to the buffer and move forward the writing offset.
    void Append(const void* data, std::size_t size) noexcept;

    //! Append another buffer to the buffer and move forward the writing offset.
    void Append(const Buffer& buf) noexcept;

    //! Ensure the buffer has enough writable space.
    void EnsureWriteableSize(std::size_t size) noexcept;

    //! Manually move forward the writing offset by a specific size.
    void HasWritten(std::size_t size) noexcept;

    //! Manually move forward the reading offset by a specific size.
    void Retrieve(std::size_t size) noexcept;

    //! Manually move forward the reading offset until it reaches the destination.
    std::size_t RetrieveUntil(const void* addr) noexcept;

    //! Manually move forward the reading offset to the end.
    std::size_t RetrieveAll() noexcept;

    /**
     * @brief Manually move forward the reading offset to the end and extract a string from the rest.
     *
     * @warning Developers should ensure that the stored bytes are printable.
     */
    std::string RetrieveAllToString() noexcept;

    //! Clear the buffer.
    void Clear() noexcept;

    //! Whether the buffer is empty.
    bool Empty() const noexcept;

protected:
    //! Get the prependable size which can be reused.
    std::size_t PrependableSize() const noexcept;

    //! Make the buffer have enough writable space.
    void MakeSpace(std::size_t size) noexcept;

    //! Get the reading offset.
    std::vector<std::byte>::iterator ReadIter() const noexcept;

    //! Get the writing offset.
    std::vector<std::byte>::iterator WriteIter() const noexcept;

    std::vector<std::byte> buf_;
    std::atomic<std::size_t> read_pos_ {0};
    std::atomic<std::size_t> write_pos_ {0};
};

/**
 * An enhanced buffer supporting I/O reading and writing.
 */
class IOBuffer : public Buffer {
public:
    using Buffer::Buffer;

    //! Read data from an I/O object.
    std::size_t ReadFrom(io::IReadWriter& io);

    //! Write data to an I/O object.
    std::size_t WriteTo(io::IReadWriter& io);
};

//! Write a string to a buffer.
Buffer& operator<<(Buffer& buf, std::string_view str) noexcept;

//! Write a buffer to another buffer.
Buffer& operator<<(Buffer& to, const Buffer& from) noexcept;

//! Write bytes to a buffer.
Buffer& operator<<(Buffer& buf, std::span<const std::byte> bytes) noexcept;

//! Write bytes to a buffer.
Buffer& operator<<(Buffer& buf,
                   std::initializer_list<std::byte> bytes) noexcept;

}  // namespace ws