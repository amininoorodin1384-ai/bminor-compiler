cmake_minimum_required(VERSION 3.16)
project(bminor_frontend LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(bminor_core
        src/lexer.cpp
        src/parser.cpp
)
target_include_directories(bminor_core PUBLIC include)

add_executable(bminor
        src/main.cpp
)
target_link_libraries(bminor PRIVATE bminor_core)

add_executable(bminor_tests
        tests/lexer_tests.cpp
)
target_link_libraries(bminor_tests PRIVATE bminor_core)

add_executable(bminor_parser_tests
        tests/parser_tests.cpp
)
target_link_libraries(bminor_parser_tests PRIVATE bminor_core)

if(MSVC)
    target_compile_options(bminor_core PRIVATE /W4)
    target_compile_options(bminor PRIVATE /W4)
    target_compile_options(bminor_tests PRIVATE /W4)
    target_compile_options(bminor_parser_tests PRIVATE /W4)
else()
    target_compile_options(bminor_core PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(bminor PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(bminor_tests PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(bminor_parser_tests PRIVATE -Wall -Wextra -Wpedantic)
endif()

enable_testing()
add_test(NAME lexer_tests COMMAND bminor_tests)
add_test(NAME parser_expression_tests COMMAND bminor_parser_tests)
add_test(NAME cli_help COMMAND bminor --help)
add_test(NAME cli_parse_smoke
        COMMAND bminor --parse "${CMAKE_CURRENT_SOURCE_DIR}/tests/program-smoke.bm")
