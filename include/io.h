/**
 * @file io.h
 * @brief I/O objects supporting reading and writing from buffers.
 *
 * @author Chen Zhenshuo (chenzs108@outlook.com)
 * @author Liu Guowen (liu.guowen@outlook.com)
 * @par GitHub
 * https://github.com/Zhuagenborn
 * @version 1.0
 * @date 2022-05-06
 *
 * @example tests/io_test.cpp
 */

#pragma once

#include "util.h"

#include <iostream>


namespace ws {

class Buffer;

namespace io {

//! An I/O reading interface interacting with buffers.
class IReader {
public:
    virtual ~IReader() noexcept = default;

    //! Read data from a buffer.
    virtual std::size_t ReadFrom(Buffer& buf) = 0;
};

//! An I/O writing interface interacting with buffers.
class IWriter {
public:
    virtual ~IWriter() noexcept = default;

    //! Write data to a buffer.
    virtual std::size_t WriteTo(Buffer& buf) = 0;
};

//! An I/O interface interacting with buffers.
class IReadWriter : public virtual IReader, public virtual IWriter {};

/**
 * @brief Null I/O.
 *
 * @details
 * It simply consumes a buffer's all readable or writable space,
 * without reading or writing anything.
 */
class Null : public virtual IReadWriter {
public:
    std::size_t WriteTo(Buffer& buf) noexcept override;

    std::size_t ReadFrom(Buffer& buf) noexcept override;
};

//! I/O for string streams.
class StringStream : public virtual IReadWriter {
public:
    /**
     * @brief Create an I/O object with string stream references.
     *
     * @param read
     * A string stream for reading.
     * Data will be read from it to a buffer.
     * @param write
     * A string stream for writing.
     * Data will be written to it from a buffer.
     *
     * @warning
     * The I/O does not maintain a copy of the stream.
     * Developers must ensure the stream is valid when reading or writing.
     */
    explicit StringStream(std::istream& read, std::ostream& write) noexcept;

    StringStream(const StringStream&) = delete;

    StringStream(StringStream&&) = delete;

    StringStream& operator=(const StringStream&) = delete;

    StringStream& operator=(StringStream&&) = delete;

    std::size_t WriteTo(Buffer& buf) noexcept override;

    std::size_t ReadFrom(Buffer& buf) noexcept override;

private:
    std::istream& read_;
    std::ostream& write_;
};

//! I/O for file descriptors.
class FileDescriptor : public virtual IReadWriter {
public:
    /**
     * @brief Create an I/O object with file descriptors.
     *
     * @param read
     * A file descriptor for reading.
     * Data will be read from it to a buffer.
     * @param write
     * A file descriptor for writing.
     * Data will be written to it from a buffer.
     *
     * @note
     * A file descriptor uses the same offset for reading and writing.
     * So the constructor needs two descriptors for reading and writing respectively.
     */
    explicit FileDescriptor(ws::FileDescriptor read,
                            ws::FileDescriptor write) noexcept;

    FileDescriptor(const FileDescriptor&) = delete;

    FileDescriptor(FileDescriptor&&) = delete;

    FileDescriptor& operator=(const FileDescriptor&) = delete;

    FileDescriptor& operator=(FileDescriptor&&) = delete;

    std::size_t WriteTo(Buffer& buf) override;

    std::size_t ReadFrom(Buffer& buf) override;

private:
    ws::FileDescriptor read_;
    ws::FileDescriptor write_;
};

}  // namespace io

}  // namespace ws