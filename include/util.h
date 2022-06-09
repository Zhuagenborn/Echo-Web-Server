/**
 * @file util.h
 * @brief Commonly used utilities.
 *
 * @author Chen Zhenshuo (chenzs108@outlook.com)
 * @par GitHub
 * https://github.com/Zhuagenborn
 * @version 1.0
 * @date 2022-04-23
 *
 * @example tests/util_test.cpp
 *
 * @todo Replace @p fmt with C++20 @p format.
 */

#pragma once

#include <fmt/format.h>
#include <yaml-cpp/yaml.h>

#include <concepts>
#include <functional>
#include <initializer_list>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

//! @p ws means "web server".
namespace ws {

//! The underlying type of file descriptors.
using FileDescriptor = int;

//! A constant value representing invalid file descriptors.
constexpr FileDescriptor invalid_file_descriptor {-1};

//! Convert a string into lower-case.
std::string StringToLower(std::string str) noexcept;

//! Convert a string into upper-case.
std::string StringToUpper(std::string str) noexcept;

/**
 * @brief Load a @p YAML node from a string.
 *
 * @param str               A @p YAML string.
 * @param required_fields   Required fields.
 *
 * @exception std::invalid_argument The node does not contain all required fields.
 */
YAML::Node LoadYamlString(
    std::string_view str,
    std::initializer_list<std::string_view> required_fields = {});

//! Throw an @p std::invalid_argument exception if a field does not exist in a @p YAML node or it is not scalar.
void ThrowIfYamlFieldIsNotScalar(const YAML::Node& node,
                                 std::string_view field);

/**
 * @brief Whether a file descriptor is valid.
 *
 * @note Assume the value of valid file descriptors should be greater than or equal to 0.
 */
constexpr bool IsValidFileDescriptor(FileDescriptor fd) noexcept {
    return fd >= 0;
}

//! Throw a @p std::system_error exception containing the last-error.
[[noreturn]] void ThrowLastSystemError();

//! Get the ID of the current thread.
std::uint32_t CurrentThreadId() noexcept;

/**
 * @brief Get a backtrace for the calling program.
 *
 * @param[out] stack    An address array.
 * @param size          The maximum number of addresses.
 * @param skip          The number of addresses to ignore.
 */
void Backtrace(std::vector<std::string>& stack, std::size_t size,
               std::size_t skip = 0) noexcept;

/**
 * @brief Get a backtrace for the calling program.
 *
 * @param size  The maximum number of addresses.
 * @param skip  The number of addresses to ignore.
 * @return A string containing the backtrace.
 */
std::string Backtrace(std::size_t size, std::size_t skip = 0,
                      std::string_view prefix = "") noexcept;

/**
 * @brief The singleton pattern interface for references.
 *
 * @tparam T    A type using the singleton pattern.
 * @tparam Args The arguments of the singleton instance, referring to one of its constructors.
 */
template <typename T, typename... Args>
class Singleton {
public:
    /**
     * @brief Get the singleton instance.
     *
     * @tparam args The construction arguments.
     */
    template <Args... args>
    static T& Instance() noexcept {
        static T ins {std::move(args)...};
        return ins;
    }
};

/**
 * @brief The singleton pattern interface for smart points.
 *
 * @tparam T    A type using the singleton pattern.
 * @tparam Args The argument types of the singleton instance, referring to one of its constructors.
 */
template <typename T, typename... Args>
class SingletonPtr {
public:
    /**
     * @brief Get the singleton instance.
     *
     * @tparam args The construction arguments.
     */
    template <Args... args>
    static std::shared_ptr<T> Instance() noexcept {
        static const std::shared_ptr<T> ins {
            std::make_shared<T>(std::move(args)...)};
        return ins;
    }
};

template <typename T, typename Cleaner = std::function<void(T)>>
requires std::is_invocable_v<Cleaner, T>
class RAII {
public:
    /**
     * @brief Create a RAII for an object.
     *
     * @param obj       An object needing to be cleaned.
     * @param cleaner   A cleaner.
     */
    explicit RAII(T obj, Cleaner cleaner) noexcept :
        obj_ {obj}, cleaner_ {cleaner} {}

    RAII(const RAII&) = delete;

    RAII(RAII&&) = delete;

    RAII& operator=(const RAII&) = delete;

    RAII& operator=(RAII&&) = delete;

    ~RAII() noexcept {
        cleaner_(obj_);
    }

    const T& Object() const noexcept {
        return obj_;
    }

private:
    T obj_;
    Cleaner cleaner_;
};

template <typename T, typename U, typename Ret = T>
concept Addable = requires(T t, U u) {
    { t + u } -> std::convertible_to<Ret>;
};

}  // namespace ws