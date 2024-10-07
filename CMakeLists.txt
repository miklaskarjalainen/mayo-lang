cmake_minimum_required(VERSION 3.28.1)
project(MyLang C)
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_EXTENSIONS off)

option(BUILD_TESTS "Build tests" OFF)

# Add dir for 'FindXLibrary.cmake' files
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

set(SOURCE_FILES
    src/cli/cli.c src/cli/cli.h

    src/common/string.c src/common/string.h  
    src/common/utils.c src/common/utils.h 
    src/common/error.h
    src/common/range.h
    
    src/lexer/token_kinds.h
    src/lexer/lexer_eat.c
    src/lexer/lexer_token.c src/lexer/lexer_token.h 
    src/lexer.c src/lexer.h 
    
    src/parser/ast_print.c src/parser/ast_print.h 
    src/parser/ast_eval.c src/parser/ast_eval.h 
    src/parser/ast_type.c src/parser/ast_type.h 
    
    src/parser/parser_eat.c 
    src/parser/parser_error.h 
    src/parser/parser_parse.c src/parser/parser_parse.h 
    src/parser.c src/parser.h

    src/variant/core_type.c src/variant/core_type.h 
    src/variant/variant.c src/variant/variant.h 

    src/file_position.c src/file_position.h 
    src/compile_error.c src/compile_error.h 
)

set(INCLUDE_DIRS
    thirdparty
)

#
if (BUILD_TESTS)
    find_package(Criterion)
    if(CRITERION_FOUND)
        # If Criterion is found, add tests
        message(STATUS "Criterion found. Adding tests.")

        # Add your source files here
        set(TEST_SRC_FILES
            tests/common/test_string.c 

            tests/lexer/lexer_token.c 
            tests/lexer/lexer.c 

            # tests/test_core_type.c 
            tests/test_utils.c
            tests/test_main.c
        )
        add_executable(tests ${SOURCE_FILES} ${TEST_SRC_FILES})
        target_include_directories(tests PRIVATE ${INCLUDE_DIRS})

        # Link the Criterion library to your test executable
        target_link_libraries(tests PRIVATE criterion)
        target_compile_options(tests PRIVATE -g -Wall -Wextra -Werror)

        # Add the tests to CTest
        # usage:
        # './bin/tests'               via binary
        # OR
        # 'ctest --output-on-failure' via ctest (in 'bin/')
        enable_testing()
        add_test(NAME Tests COMMAND tests)
    else()
        message(WARNING "Criterion not found. Tests will not be built.")
    endif()
endif()

# Set the name of the executable
add_executable(my_lang src/main.c ${SOURCE_FILES})
target_include_directories(my_lang PRIVATE ${INCLUDE_DIRS})

# Set compiler flags for debug build
target_compile_options(my_lang PRIVATE -g -Wall -Wextra -Werror -Wmissing-prototypes -Wstrict-prototypes -pedantic)
