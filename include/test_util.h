/**
 * @file test_util.h
 * @brief Commonly used utilities for unit tests.
 *
 * @author Chen Zhenshuo (chenzs108@outlook.com)
 * @par GitHub
 * https://github.com/Zhuagenborn
 * @version 1.0
 * @date 2022-05-05
 */

#pragma once

#include "log.h"
#include "util.h"

#include <string>
#include <utility>


namespace ws::test {

/**
 * @brief Get the current test name.
 *
 * @details
 * The format is @p SuiteName.CaseName.
 * @code {.cpp}
 * TEST(SuiteName, CaseName) {
 *     // ...
 * }
 * @endcode
 */
std::string TestName() noexcept;

/**
 * @brief Create an unique temporary file for the current test.
 *
 * @return The file descriptor and path.
 *
 * @exception std::system_error Failed to create a temporary file.
 */
std::pair<FileDescriptor, std::string> CreateTempTestFile();

//! Get a logger for tests.
log::Logger::Ptr TestLogger() noexcept;

}  // namespace ws::test