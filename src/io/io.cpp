#include "io.h"
#include "containers/buffer.h"

#include <sys/uio.h>
#include <unistd.h>

#include <array>


namespace ws::io {

std::size_t Null::WriteTo(Buffer& buf) noexcept {
    const auto size {buf.WritableSize()};
    buf.HasWritten(size);
    return size;
}

std::size_t Null::ReadFrom(Buffer& buf) noexcept {
    return buf.RetrieveAll();
}

StringStream::StringStream(std::istream& read, std::ostream& write) noexcept :
    read_ {read}, write_ {write} {}

std::size_t StringStream::WriteTo(Buffer& buf) noexcept {
    std::string str;
    read_ >> str;
    buf.Append(str);
    return str.length();
}

std::size_t StringStream::ReadFrom(Buffer& buf) noexcept {
    const std::string str {buf.RetrieveAllToString()};
    write_ << str;
    return str.length();
}

FileDescriptor::FileDescriptor(const ws::FileDescriptor read,
                               const ws::FileDescriptor write) noexcept :
    read_ {read}, write_ {write} {}

std::size_t FileDescriptor::WriteTo(Buffer& buf) {
    const auto buf_bytes {buf.WritableBytes()};
    std::array<std::byte, 0x10000> ext_bytes;

    // Use an additional buffer to read more data.
    // If the writable size of the input buffer is very small,
    // the remaining data will be placed in the additional buffer,
    // then appended to the input buffer.
    const std::array bufs {
        iovec {.iov_base = buf_bytes.data(), .iov_len = buf_bytes.size_bytes()},
        iovec {.iov_base = ext_bytes.data(), .iov_len = ext_bytes.size()}};

    const auto size {readv(read_, bufs.data(), bufs.size())};
    if (size < 0) {
        ThrowLastSystemError();
    } else if (size <= buf_bytes.size_bytes()) {
        buf.HasWritten(size);
    } else {
        buf.HasWritten(buf_bytes.size_bytes());
        buf.Append({ext_bytes.cbegin(),
                    ext_bytes.cbegin() + (size - buf_bytes.size_bytes())});
    }

    return size;
}

std::size_t FileDescriptor::ReadFrom(Buffer& buf) {
    const auto bytes {buf.ReadableBytes()};
    if (const auto size {write(write_, bytes.data(), bytes.size_bytes())};
        size >= 0) {
        buf.Retrieve(size);
        return size;
    } else {
        ThrowLastSystemError();
    }
}

}  // namespace ws::io