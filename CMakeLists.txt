cmake_minimum_required(VERSION 3.16)
project(minisql LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---- Library (core) ----
add_library(imd_core STATIC
    src/lexer.cpp
    src/parser.cpp
    src/executor.cpp
    src/renderer.cpp
)
target_include_directories(imd_core PUBLIC ${CMAKE_SOURCE_DIR}/include)

# ---- CLI app ----
add_executable(db app/main.cpp)
target_link_libraries(db PRIVATE imd_core)

# ---- GoogleTest ----
include(FetchContent)
FetchContent_Declare(googletest DOWNLOAD_EXTRACT_TIMESTAMP TRUE URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)
# For MinGW on Windows, avoid overriding parent compiler flags
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

add_executable(gtests
    tests/test_gtest.cpp
)
target_link_libraries(gtests PRIVATE imd_core GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(gtests)

