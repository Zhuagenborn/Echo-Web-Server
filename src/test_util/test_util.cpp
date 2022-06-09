#include "test_util.h"

#include <gtest/gtest.h>

#include <filesystem>


namespace ws::test {

std::string TestName() noexcept {
    using namespace testing;
    const std::string suite_name {
        UnitTest::GetInstance()->current_test_info()->test_suite_name()};
    const std::string case_name {
        UnitTest::GetInstance()->current_test_info()->name()};
    return suite_name + "." + case_name;
}

std::pair<FileDescriptor, std::string> CreateTempTestFile() {
    static constexpr std::string_view suffix {"-XXXXXX"};

    const auto file {TestName() + suffix.data()};
    std::string path {std::filesystem::temp_directory_path().append(file)};
    if (const auto fd {mkstemp(path.data())}; IsValidFileDescriptor(fd)) {
        return {fd, path};
    } else {
        ThrowLastSystemError();
    }
}

}  // namespace ws::test