#include "util.h"

#include <execinfo.h>
#include <sys/syscall.h>
#include <unistd.h>

#include <algorithm>
#include <cerrno>
#include <sstream>
#include <system_error>


namespace ws {

std::string StringToLower(std::string str) noexcept {
    std::ranges::transform(
        str.begin(), str.end(), str.begin(),
        [](const unsigned char c) noexcept { return std::tolower(c); });
    return str;
}

std::string StringToUpper(std::string str) noexcept {
    std::ranges::transform(
        str.begin(), str.end(), str.begin(),
        [](const unsigned char c) noexcept { return std::toupper(c); });
    return str;
}

YAML::Node LoadYamlString(
    const std::string_view str,
    const std::initializer_list<std::string_view> required_fields) {
    const YAML::Node node {YAML::Load(str.data())};
    std::ranges::for_each(
        required_fields, [&node, &str](const std::string_view field) {
            if (!node[field.data()]) {
                throw std::invalid_argument {
                    fmt::format("Invalid YAML value: '{}'", str)};
            }
        });
    return node;
}

void ThrowIfYamlFieldIsNotScalar(const YAML::Node& node,
                                 const std::string_view field) {
    if (!node[field.data()] || !node[field.data()].IsScalar()) {
        throw std::invalid_argument {
            fmt::format("Invalid YAML field: '{}'", field)};
    }
}

void ThrowLastSystemError() {
    throw std::system_error {errno, std::system_category()};
}

std::uint32_t CurrentThreadId() noexcept {
    return static_cast<std::uint32_t>(syscall(SYS_gettid));
}

void Backtrace(std::vector<std::string>& stack, const std::size_t size,
               const std::size_t skip) noexcept {
    using VoidPtr = void*;
    std::unique_ptr<VoidPtr[]> buffer {new VoidPtr[size] {}};
    const auto ret_size {backtrace(buffer.get(), static_cast<int>(size))};
    char** const stack_strs {backtrace_symbols(buffer.get(), ret_size)};
    if (stack_strs) {
        for (auto i {skip}; i < ret_size; ++i) {
            stack.push_back(stack_strs[i]);
        }

        free(stack_strs);
    }
}

std::string Backtrace(const std::size_t size, const std::size_t skip,
                      const std::string_view prefix) noexcept {
    std::vector<std::string> stack;
    Backtrace(stack, size, skip);
    std::ostringstream backtrace;
    std::ranges::for_each(stack,
                          [&backtrace, &prefix](const auto& call) noexcept {
                              backtrace << prefix << call << "\n";
                          });

    return backtrace.str();
}

}  // namespace ws