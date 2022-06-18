#include "util.h"

#include <execinfo.h>
#include <sys/syscall.h>
#include <unistd.h>

#include <algorithm>
#include <cerrno>
#include <iterator>
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

std::string ReplaceAllSubstring(std::string_view str,
                                const std::string_view from,
                                const std::string_view to) noexcept {
    std::ostringstream ss;
    while (!str.empty()) {
        const auto begin {str.find(from)};
        ss << str.substr(0, begin);
        if (begin != std::string_view::npos) {
            ss << to;
            str = str.substr(begin + from.length());
        } else {
            break;
        }
    }

    return ss.str();
}

std::vector<std::string> SplitString(const std::string& str,
                                     const std::regex& pattern) noexcept {
    std::vector<std::string> strs;
    const std::sregex_token_iterator begin {str.cbegin(), str.cend(), pattern,
                                            -1};
    std::copy(begin, std::sregex_token_iterator {}, std::back_inserter(strs));
    return strs;
}

std::vector<std::string> SplitStringToLines(const std::string& str) noexcept {
    static const std::regex pattern {"\r*\n"};
    return SplitString(str, pattern);
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