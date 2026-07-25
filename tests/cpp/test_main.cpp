#include "test_support.hpp"

#include <exception>
#include <iostream>

int main()
{
    int failures = 0;

    for (const auto& test : parallelpix::test::registry())
    {
        try
        {
            test.function();
            std::cout << "[PASS] " << test.name << '\n';
        }
        catch (const std::exception& error)
        {
            ++failures;
            std::cerr << "[FAIL] " << test.name << ": " << error.what() << '\n';
        }
    }

    std::cout << parallelpix::test::registry().size() << " tests, "
              << failures << " failures\n";
    return failures == 0 ? 0 : 1;
}
