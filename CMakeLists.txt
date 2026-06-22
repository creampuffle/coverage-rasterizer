cmake_minimum_required(VERSION 3.16)
project(coverage_rasterizer VERSION 0.1.0 LANGUAGES C)

option(CR_BUILD_TESTS "Build the host-side tests" ON)

add_library(coverage_rasterizer STATIC src/coverage_rasterizer.c)
target_include_directories(coverage_rasterizer PUBLIC include)
target_compile_features(coverage_rasterizer PUBLIC c_std_11)

if(MSVC)
    target_compile_options(coverage_rasterizer PRIVATE /W4)
else()
    target_compile_options(coverage_rasterizer PRIVATE
        -Wall -Wextra -Wpedantic -Wconversion -Wshadow
        -ffreestanding -fno-builtin
    )
endif()

if(CR_BUILD_TESTS)
    enable_testing()
    add_executable(coverage_rasterizer_tests tests/test_coverage_rasterizer.c)
    target_link_libraries(coverage_rasterizer_tests PRIVATE coverage_rasterizer)
    target_compile_features(coverage_rasterizer_tests PRIVATE c_std_11)
    if(MSVC)
        target_compile_options(coverage_rasterizer_tests PRIVATE /W4)
    else()
        target_compile_options(coverage_rasterizer_tests PRIVATE
            -Wall -Wextra -Wpedantic -Wconversion -Wshadow
        )
    endif()
    add_test(NAME coverage_rasterizer_tests COMMAND coverage_rasterizer_tests)
endif()
