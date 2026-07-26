cmake_minimum_required(VERSION 3.20)

project(ParallelPix LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(PARALLELPIX_CUDA "AUTO" CACHE STRING
    "Build the CUDA backend: AUTO, ON, or OFF")
set_property(CACHE PARALLELPIX_CUDA PROPERTY STRINGS AUTO ON OFF)
string(TOUPPER "${PARALLELPIX_CUDA}" PARALLELPIX_CUDA_MODE)
if(NOT PARALLELPIX_CUDA_MODE MATCHES "^(AUTO|ON|OFF)$")
    message(FATAL_ERROR
        "PARALLELPIX_CUDA must be AUTO, ON, or OFF.")
endif()

set(PARALLELPIX_HAS_CUDA OFF)
if(NOT PARALLELPIX_CUDA_MODE STREQUAL "OFF")
    include(CheckLanguage)
    check_language(CUDA)
    if(CMAKE_CUDA_COMPILER)
        enable_language(CUDA)
        find_package(CUDAToolkit REQUIRED)
        if(WIN32)
            find_file(PARALLELPIX_CUDART_DLL
                NAMES cudart64_12.dll
                PATHS "${CUDAToolkit_BIN_DIR}"
                NO_DEFAULT_PATH
                REQUIRED
            )
        endif()
        set(PARALLELPIX_HAS_CUDA ON)
        if(NOT DEFINED CMAKE_CUDA_ARCHITECTURES)
            set(CMAKE_CUDA_ARCHITECTURES 120 CACHE STRING
                "CUDA architectures for ParallelPix")
        endif()
        message(STATUS
            "ParallelPix CUDA backend enabled for architectures: "
            "${CMAKE_CUDA_ARCHITECTURES}")
    elseif(PARALLELPIX_CUDA_MODE STREQUAL "ON")
        message(FATAL_ERROR
            "PARALLELPIX_CUDA=ON but no CUDA compiler was found.")
    else()
        message(STATUS
            "CUDA compiler not found; building CPU-only ParallelPix.")
    endif()
endif()

find_package(OpenCV CONFIG REQUIRED COMPONENTS core imgcodecs)

function(parallelpix_enable_warnings target)
    if(MSVC)
        target_compile_options(${target} PRIVATE
            $<$<COMPILE_LANGUAGE:CXX>:/W4>
            $<$<COMPILE_LANGUAGE:CXX>:/permissive->
            $<$<COMPILE_LANGUAGE:CXX>:/utf-8>
        )
    else()
        target_compile_options(${target} PRIVATE
            $<$<COMPILE_LANGUAGE:CXX>:-Wall>
            $<$<COMPILE_LANGUAGE:CXX>:-Wextra>
            $<$<COMPILE_LANGUAGE:CXX>:-Wpedantic>
        )
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
    src/common/validation/preparation.cpp
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

if(PARALLELPIX_HAS_CUDA)
    add_library(parallelpix_cuda STATIC
        src/cuda/processor/batch_executor.cpp
        src/cuda/processor/batch_packer.cpp
        src/cuda/processor/processor.cpp
        src/cuda/runtime/resources.cpp
        src/cuda/runtime/runtime.cpp
        src/cuda/kernels/process_batch.cu
    )
    target_include_directories(parallelpix_cuda
        PUBLIC
            ${CMAKE_CURRENT_SOURCE_DIR}/include
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/src/cuda
    )
    target_link_libraries(parallelpix_cuda
        PUBLIC
            parallelpix_processing
        PRIVATE
            CUDA::cudart
    )
    set_target_properties(parallelpix_cuda PROPERTIES
        CUDA_STANDARD 17
        CUDA_STANDARD_REQUIRED ON
        CUDA_EXTENSIONS OFF
    )
    if(WIN32)
        add_custom_command(TARGET parallelpix_cuda POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${PARALLELPIX_CUDART_DLL}"
                "$<TARGET_FILE_DIR:parallelpix_cuda>"
            COMMENT "Copying the CUDA runtime beside ParallelPix binaries"
        )
    endif()
    parallelpix_enable_warnings(parallelpix_cuda)
endif()

set(PARALLELPIX_BENCHMARK_SOURCES
    src/benchmark/reporting/reporting.cpp
    src/benchmark/runner/experiment.cpp
    src/benchmark/runner/runner.cpp
    src/benchmark/runner/runner_io.cpp
    src/benchmark/runner/sequential_executor.cpp
    src/benchmark/statistics/statistics.cpp
    src/benchmark/validation/validation.cpp
    src/pipeline/benchmark_pipeline.cpp
)
if(PARALLELPIX_HAS_CUDA)
    list(APPEND PARALLELPIX_BENCHMARK_SOURCES
        src/benchmark/runner/cuda_executor.cpp
    )
else()
    list(APPEND PARALLELPIX_BENCHMARK_SOURCES
        src/benchmark/runner/cuda_executor_stub.cpp
    )
endif()

add_library(parallelpix_benchmark STATIC
    ${PARALLELPIX_BENCHMARK_SOURCES}
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
if(PARALLELPIX_HAS_CUDA)
    target_link_libraries(parallelpix_benchmark PRIVATE parallelpix_cuda)
endif()
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

    if(PARALLELPIX_HAS_CUDA)
        add_executable(parallelpix_m6_tests
            tests/cpp/test_main.cpp
            tests/cpp/cuda/test_faults.cpp
            tests/cpp/cuda/test_processor.cpp
            tests/cpp/cuda/test_runtime.cpp
        )
        target_include_directories(parallelpix_m6_tests
            PRIVATE
                ${CMAKE_CURRENT_SOURCE_DIR}/tests/cpp
                ${CMAKE_CURRENT_SOURCE_DIR}/tests/cpp/sequential
                ${CMAKE_CURRENT_SOURCE_DIR}/src/cuda
        )
        target_link_libraries(parallelpix_m6_tests
            PRIVATE
                parallelpix_cuda
                parallelpix_sequential
        )
        parallelpix_enable_warnings(parallelpix_m6_tests)

        add_test(NAME parallelpix_m6_tests COMMAND parallelpix_m6_tests)
    endif()

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
