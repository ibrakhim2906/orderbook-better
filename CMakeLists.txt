cmake_minimum_required(VERSION 3.21)
project(orderbook CXX)

enable_testing()

string(TOUPPER "${CMAKE_BUILD_TYPE}" _bt)
message(STATUS "CXX flags: ${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_${_bt}}")

# ---- core library
add_library(orderbook_core STATIC src/book.cpp)
target_include_directories(orderbook_core PUBLIC include)
target_compile_features(orderbook_core PUBLIC cxx_std_20)
target_compile_options(orderbook_core PUBLIC -Wall -Wextra -Wpedantic -Werror)

# ---- driver
add_executable(app src/main.cpp)
target_link_libraries(app PRIVATE orderbook_core)

# ---- tests
find_package(GTest CONFIG REQUIRED)
add_executable(tests tests/test_main.cpp
        tests/test_types.cpp
        tests/test_book.cpp
        tests/test_add.cpp
        tests/test_cancel_reduce.cpp
)
target_link_libraries(tests PRIVATE orderbook_core GTest::gtest GTest::gtest_main)
include(GoogleTest)
gtest_discover_tests(tests)

# ---- benchmarks
find_package(benchmark CONFIG REQUIRED)
add_executable(bench bench/bench_main.cpp)
target_link_libraries(bench PRIVATE orderbook_core benchmark::benchmark)
