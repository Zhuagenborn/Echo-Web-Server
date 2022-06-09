#include "buffer.h"
#include "io.h"

#include <algorithm>
#include <cassert>


namespace ws {

Buffer::Buffer(const std::size_t size) noexcept : buf_(size) {}

Buffer::Buffer(const std::span<const std::byte> bytes) noexcept {
    Append(bytes);
}

Buffer::Buffer(const std::initializer_list<std::byte> bytes) noexcept {
    Append(bytes.begin(), bytes.size());
}

Buffer::Buffer(const std::string_view str) noexcept {
    Append(str);
}

Buffer::Buffer(const Buffer& o) noexcept :
    buf_ {o.buf_},
    read_pos_ {o.read_pos_.load()},
    write_pos_ {o.write_pos_.load()} {}

Buffer::Buffer(Buffer&& o) noexcept :
    buf_ {std::move(o.buf_)},
    read_pos_ {o.read_pos_.load()},
    write_pos_ {o.write_pos_.load()} {}

Buffer& Buffer::operator=(const Buffer& o) noexcept {
    buf_ = o.buf_;
    read_pos_ = o.read_pos_.load();
    write_pos_ = o.write_pos_.load();
    return *this;
}

Buffer& Buffer::operator=(Buffer&& o) noexcept {
    buf_ = std::move(o.buf_);
    read_pos_ = o.read_pos_.load();
    write_pos_ = o.write_pos_.load();
    return *this;
}

std::size_t Buffer::WritableSize() const noexcept {
    return buf_.size() - write_pos_;
}

std::size_t Buffer::ReadableSize() const noexcept {
    return write_pos_ - read_pos_;
}

std::size_t Buffer::PrependableSize() const noexcept {
    return read_pos_;
}

std::optional<std::byte> Buffer::Peek() const noexcept {
    return Empty() ? std::nullopt : std::optional {*ReadIter()};
}

void Buffer::Append(const std::initializer_list<std::byte> bytes) noexcept {
    Append(bytes.begin(), bytes.size());
}

void Buffer::Append(const std::span<const std::byte> bytes) noexcept {
    Append(bytes.data(), bytes.size_bytes());
}

void Buffer::Append(const std::string_view str,
                    const std::optional<NewLine> new_line) noexcept {
    std::string full_str {str};
    if (new_line.has_value()) {
        if (new_line == NewLine::LF) {
            full_str += "\n";
        } else if (new_line == NewLine::CRLF) {
            full_str += "\r\n";
        } else {
            assert(false);
        }
    }

    Append(full_str.data(), full_str.length());
}

void Buffer::Append(const void* const data, const std::size_t size) noexcept {
    if (size == 0) {
        return;
    }

    assert(data);

    EnsureWriteableSize(size);
    const auto base {reinterpret_cast<const std::byte*>(data)};
    std::copy(base, base + size, WriteIter());
    HasWritten(size);

    assert(ReadableSize() >= size);
}

void Buffer::Append(const Buffer& buf) noexcept {
    Append(buf.ReadableBytes());
}

void Buffer::EnsureWriteableSize(const std::size_t size) noexcept {
    if (WritableSize() < size) {
        MakeSpace(size);
    }

    assert(WritableSize() >= size);
}

void Buffer::MakeSpace(const std::size_t size) noexcept {
    if (WritableSize() + PrependableSize() < size) {
        buf_.resize(write_pos_ + size);
    } else {
        const auto readable_size {ReadableSize()};
        std::copy(ReadIter(), WriteIter(), buf_.begin());
        read_pos_ = 0;
        write_pos_ = read_pos_ + readable_size;

        assert(readable_size == ReadableSize());
    }
}

std::span<const std::byte> Buffer::ReadableBytes() const noexcept {
    return {ReadIter().base(), ReadableSize()};
}

std::string Buffer::ReadableString() const noexcept {
    return {reinterpret_cast<char*>(ReadIter().base()), ReadableSize()};
}

std::span<std::byte> Buffer::WritableBytes() const noexcept {
    return {WriteIter().base(), WritableSize()};
}

void Buffer::HasWritten(const std::size_t size) noexcept {
    assert(WritableSize() >= size);
    write_pos_ += size;
}

void Buffer::Retrieve(const std::size_t size) noexcept {
    assert(ReadableSize() >= size);
    read_pos_ += size;
}

std::size_t Buffer::RetrieveAll() noexcept {
    const auto read_size {ReadableSize()};
    Clear();
    return read_size;
}

std::string Buffer::RetrieveAllToString() noexcept {
    const auto str {ReadableString()};
    Clear();
    return str;
}

std::size_t Buffer::RetrieveUntil(const void* const addr) noexcept {
    const auto end {static_cast<const std::byte*>(addr)};
    const auto begin {ReadIter().base()};

    assert(begin <= end);

    const auto read_size {end - begin};
    Retrieve(read_size);
    return read_size;
}

void Buffer::Clear() noexcept {
    read_pos_ = 0;
    write_pos_ = 0;
    assert(Empty());
}

bool Buffer::Empty() const noexcept {
    return ReadableSize() == 0;
}

std::vector<std::byte>::iterator Buffer::ReadIter() const noexcept {
    return const_cast<Buffer*>(this)->buf_.begin() + read_pos_;
}

std::vector<std::byte>::iterator Buffer::WriteIter() const noexcept {
    return const_cast<Buffer*>(this)->buf_.begin() + write_pos_;
}

std::size_t IOBuffer::ReadFrom(io::IReadWriter& io) {
    return io.WriteTo(*this);
}

std::size_t IOBuffer::WriteTo(io::IReadWriter& io) {
    return io.ReadFrom(*this);
}

Buffer& operator<<(Buffer& buf, const std::string_view str) noexcept {
    buf.Append(str);
    return buf;
}

Buffer& operator<<(Buffer& to, const Buffer& from) noexcept {
    to.Append(from);
    return to;
}

Buffer& operator<<(Buffer& buf,
                   const std::span<const std::byte> bytes) noexcept {
    buf.Append(bytes);
    return buf;
}

Buffer& operator<<(Buffer& buf,
                   const std::initializer_list<std::byte> bytes) noexcept {
    buf.Append(bytes);
    return buf;
}

}  // namespace ws