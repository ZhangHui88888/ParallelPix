#pragma once

#include <functional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace parallelpix::test {

struct TestCase
{
    std::string name;
    std::function<void()> function;
};

inline std::vector<TestCase>& registry()
{
    static std::vector<TestCase> tests;
    return tests;
}

class Registrar
{
public:
    Registrar(std::string name, std::function<void()> function)
    {
        registry().push_back({std::move(name), std::move(function)});
    }
};

template <typename Actual, typename Expected>
void require_equal(
    const Actual& actual,
    const Expected& expected,
    const char* actual_expression,
    const char* expected_expression)
{
    if (!(actual == expected))
    {
        std::ostringstream message;
        message << "expected " << actual_expression << " == " << expected_expression;
        throw std::runtime_error(message.str());
    }
}

inline void require(bool condition, const char* expression)
{
    if (!condition)
    {
        throw std::runtime_error(std::string("expected true: ") + expression);
    }
}

}  // namespace parallelpix::test

#define PP_CONCAT_INNER(left, right) left##right
#define PP_CONCAT(left, right) PP_CONCAT_INNER(left, right)

#define PP_TEST(name)                                                            \
    static void PP_CONCAT(test_function_, __LINE__)();                           \
    static ::parallelpix::test::Registrar PP_CONCAT(test_registrar_, __LINE__)(  \
        name, PP_CONCAT(test_function_, __LINE__));                              \
    static void PP_CONCAT(test_function_, __LINE__)()

#define PP_REQUIRE(expression) \
    ::parallelpix::test::require(static_cast<bool>(expression), #expression)

#define PP_REQUIRE_EQ(actual, expected) \
    ::parallelpix::test::require_equal((actual), (expected), #actual, #expected)
