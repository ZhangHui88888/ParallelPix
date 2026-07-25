cmake_minimum_required(VERSION 3.20)

project(ParallelPix LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(OpenCV CONFIG REQUIRED COMPONENTS core imgcodecs)

function(parallelpix_enable_warnings target)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /permissive- /utf-8)
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endfunction()

add_library(parallelpix_m2 STATIC
    src/cli/cli_parser.cpp
    src/planning/benchmark_plan.cpp
    src/controller/controller.cpp
)
target_include_directories(parallelpix_m2
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)
parallelpix_enable_warnings(parallelpix_m2)

add_library(parallelpix_io STATIC
    src/io/catalog/catalog.cpp
    src/io/decode/batch.cpp
    src/io/decode/decode.cpp
    src/io/decode/watermark.cpp
    src/io/encode/encode.cpp
    src/io/encode/output_directory.cpp
)
target_include_directories(parallelpix_io
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_link_libraries(parallelpix_io
    PRIVATE
        opencv_core
        opencv_imgcodecs
)
parallelpix_enable_warnings(parallelpix_io)

add_library(parallelpix_processing STATIC
    src/common/effects/effects.cpp
    src/common/geometry/center_crop.cpp
    src/common/resize/bilinear.cpp
)
target_include_directories(parallelpix_processing
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)
parallelpix_enable_warnings(parallelpix_processing)

add_library(parallelpix_sequential STATIC
    src/sequential/processor/process_image.cpp
    src/sequential/processor/processor.cpp
)
target_include_directories(parallelpix_sequential
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_link_libraries(parallelpix_sequential
    PUBLIC
        parallelpix_processing
)
parallelpix_enable_warnings(parallelpix_sequential)

add_library(parallelpix_benchmark STATIC
    src/benchmark/reporting/reporting.cpp
    src/benchmark/runner/experiment.cpp
    src/benchmark/runner/runner.cpp
    src/benchmark/runner/runner_io.cpp
    src/benchmark/runner/sequential_executor.cpp
    src/benchmark/statistics/statistics.cpp
    src/benchmark/validation/validation.cpp
    src/pipeline/benchmark_pipeline.cpp
)
target_include_directories(parallelpix_benchmark
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_link_libraries(parallelpix_benchmark
    PUBLIC
        parallelpix_m2
    PRIVATE
        parallelpix_io
        parallelpix_sequential
)
parallelpix_enable_warnings(parallelpix_benchmark)

add_executable(parallelpix
    src/cli/main.cpp
)
target_link_libraries(parallelpix PRIVATE parallelpix_benchmark)
parallelpix_enable_warnings(parallelpix)

include(CTest)

if(BUILD_TESTING)
    add_executable(parallelpix_m2_tests
        tests/cpp/test_main.cpp
        tests/cpp/cli/test_cli.cpp
        tests/cpp/planning/test_benchmark_plan.cpp
        tests/cpp/controller/test_controller.cpp
    )
    target_link_libraries(parallelpix_m2_tests PRIVATE parallelpix_m2)
    parallelpix_enable_warnings(parallelpix_m2_tests)

    add_test(NAME parallelpix_m2_tests COMMAND parallelpix_m2_tests)

    add_executable(parallelpix_m3_tests
        tests/cpp/test_main.cpp
        tests/cpp/io/test_catalog_and_decode.cpp
        tests/cpp/io/test_decode_and_batch.cpp
        tests/cpp/io/test_output.cpp
    )
    target_compile_definitions(parallelpix_m3_tests
        PRIVATE
            PARALLELPIX_TEST_FIXTURES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures"
    )
    target_include_directories(parallelpix_m3_tests
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/cpp
    )
    target_link_libraries(parallelpix_m3_tests PRIVATE parallelpix_io)
    parallelpix_enable_warnings(parallelpix_m3_tests)

    add_test(NAME parallelpix_m3_tests COMMAND parallelpix_m3_tests)

    add_executable(parallelpix_m4_tests
        tests/cpp/test_main.cpp
        tests/cpp/common/test_processing_semantics.cpp
        tests/cpp/sequential/test_processor.cpp
        tests/cpp/sequential/test_validation.cpp
        tests/cpp/sequential/test_integration.cpp
    )
    target_compile_definitions(parallelpix_m4_tests
        PRIVATE
            PARALLELPIX_TEST_FIXTURES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures"
    )
    target_include_directories(parallelpix_m4_tests
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/cpp/io
    )
    target_link_libraries(parallelpix_m4_tests
        PRIVATE
            parallelpix_io
            parallelpix_sequential
    )
    parallelpix_enable_warnings(parallelpix_m4_tests)

    add_test(NAME parallelpix_m4_tests COMMAND parallelpix_m4_tests)

    add_executable(parallelpix_m7_tests
        tests/cpp/test_main.cpp
        tests/cpp/benchmark/test_reporting.cpp
        tests/cpp/benchmark/test_runner.cpp
        tests/cpp/benchmark/test_statistics.cpp
        tests/cpp/benchmark/test_validation.cpp
        tests/cpp/pipeline/test_pipeline_factory.cpp
    )
    target_compile_definitions(parallelpix_m7_tests
        PRIVATE
            PARALLELPIX_TEST_FIXTURES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures"
    )
    target_include_directories(parallelpix_m7_tests
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/cpp/io
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/cpp/sequential
    )
    target_link_libraries(parallelpix_m7_tests
        PRIVATE
            parallelpix_benchmark
            parallelpix_io
    )
    parallelpix_enable_warnings(parallelpix_m7_tests)

    add_test(NAME parallelpix_m7_tests COMMAND parallelpix_m7_tests)

    if(WIN32)
        find_program(PARALLELPIX_POWERSHELL
            NAMES pwsh powershell
            REQUIRED
        )

        function(add_parallelpix_process_test name scenario)
            add_test(
                NAME ${name}
                COMMAND
                    ${PARALLELPIX_POWERSHELL}
                    -NoProfile
                    -ExecutionPolicy Bypass
                    -File ${CMAKE_CURRENT_SOURCE_DIR}/tests/powershell/assert_cli_process.ps1
                    -Executable $<TARGET_FILE:parallelpix>
                    -Scenario ${scenario}
                    -FixtureRoot ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures
            )
        endfunction()

        add_parallelpix_process_test(parallelpix_help help)
        add_parallelpix_process_test(parallelpix_invalid_command invalid)
        add_parallelpix_process_test(parallelpix_sequential_benchmark sequential)
        add_parallelpix_process_test(parallelpix_partial_benchmark mixed)
        add_parallelpix_process_test(parallelpix_utf8_arguments utf8)
    endif()
endif()
