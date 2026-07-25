#include "parallelpix/controller/controller.hpp"
#include "parallelpix/pipeline/pipeline_factory.hpp"

#include <iostream>
#include <string>
#include <string_view>
#include <vector>

#if defined(_WIN32)
#include <Windows.h>

#include <cwchar>
#endif

namespace {

int run_with_utf8_arguments(const std::vector<std::string>& owned_arguments)
{
    std::vector<std::string_view> argument_views;
    argument_views.reserve(owned_arguments.size());
    for (const auto& argument : owned_arguments)
    {
        argument_views.emplace_back(argument);
    }

    auto pipeline = parallelpix::m2::make_benchmark_pipeline();
    return parallelpix::m2::run_cli(
        argument_views, *pipeline, std::cout, std::cerr);
}

#if defined(_WIN32)
std::string wide_to_utf8(const wchar_t* value)
{
    const auto character_count = std::wcslen(value);
    if (character_count == 0)
    {
        return {};
    }

    const auto utf8_size = WideCharToMultiByte(
        CP_UTF8,
        WC_ERR_INVALID_CHARS,
        value,
        static_cast<int>(character_count),
        nullptr,
        0,
        nullptr,
        nullptr);
    if (utf8_size <= 0)
    {
        return {};
    }

    std::string result(static_cast<std::size_t>(utf8_size), '\0');
    const auto converted = WideCharToMultiByte(
        CP_UTF8,
        WC_ERR_INVALID_CHARS,
        value,
        static_cast<int>(character_count),
        result.data(),
        utf8_size,
        nullptr,
        nullptr);
    if (converted != utf8_size)
    {
        return {};
    }
    return result;
}
#endif

}  // namespace

#if defined(_WIN32)
int wmain(int argc, wchar_t* argv[])
{
    SetConsoleOutputCP(CP_UTF8);

    std::vector<std::string> arguments;
    arguments.reserve(argc > 1 ? static_cast<std::size_t>(argc - 1) : 0);
    for (int index = 1; index < argc; ++index)
    {
        arguments.push_back(wide_to_utf8(argv[index]));
    }
    return run_with_utf8_arguments(arguments);
}
#else
int main(int argc, char* argv[])
{
    std::vector<std::string> arguments;
    arguments.reserve(argc > 1 ? static_cast<std::size_t>(argc - 1) : 0);
    for (int index = 1; index < argc; ++index)
    {
        arguments.emplace_back(argv[index]);
    }
    return run_with_utf8_arguments(arguments);
}
#endif
