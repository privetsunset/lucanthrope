cmake_minimum_required(VERSION 3.20)
project(lucanthrope LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# C++17 is required
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(lucanthrope "")
target_include_directories(lucanthrope PRIVATE "include/lucanthrope"  "lib/" INTERFACE "include/")
target_compile_options(lucanthrope PRIVATE "-Wall" "-O3" "-Wextra" "-pedantic-errors")
target_sources(lucanthrope
    PRIVATE
    "lib/storage/RAMDirectory.cpp"
    "lib/IO/IndexInput.cpp"
    "lib/IO/IndexOutput.cpp"

)

add_executable(Document_test "tests/Document_test.cpp")
target_link_libraries(Document_test lucanthrope)
target_compile_options(Document_test PRIVATE "-Wall" "-O3" "-Wextra" "-pedantic-errors")

add_executable(CharTokenizer_test "tests/CharTokenizer_test.cpp")
target_link_libraries(CharTokenizer_test lucanthrope)
target_compile_options(CharTokenizer_test PRIVATE "-Wall" "-O3" "-Wextra" "-pedantic-errors")

add_executable(StopAnalyzer_test "tests/StopAnalyzer_test.cpp")
target_link_libraries(StopAnalyzer_test lucanthrope)
target_compile_options(StopAnalyzer_test PRIVATE "-Wall" "-O3" "-Wextra" "-pedantic-errors")

add_executable(RAMDirectory_single_thread_test "tests/RAMDirectory_single_thread_test.cpp")
target_link_libraries(RAMDirectory_single_thread_test lucanthrope)
target_compile_options(RAMDirectory_single_thread_test PRIVATE "-Wall" "-O3" "-Wextra" "-pedantic-errors")