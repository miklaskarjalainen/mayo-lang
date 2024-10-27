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
    src/common/sym_table.c src/common/sym_table.h  
    src/common/utils.c src/common/utils.h 
    src/common/arena.c src/common/arena.h 
    src/common/stats.h
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
    
    src/semantics.c src/semantics.h

    src/optimizer/optimize.h src/optimizer/optimize_ast.c

    src/backend_qbe.c src/backend_qbe.h
    src/backend/impl_gen.c

    src/variant/variant.c src/variant/variant.h 

    src/file_position.c src/file_position.h 
    src/compile_error.c src/compile_error.h 
)

set(INCLUDE_DIRS
    thirdparty
    src
)

#
if (BUILD_TESTS)
    # Add your source files here
    set(TEST_SRC_FILES
        tests/common/test_arena.c
        tests/common/test_string.c
        tests/common/test_utils.c
        
        tests/lexer/test_lexer.c

        tests/parser/test_parser.c
    
        tests/test_main.c
    )

    find_package(Criterion)
    if (NOT CRITERION_FOUND) 
        message(STATUS "Pulling Criterion from git!")

        include(FetchContent)
        FetchContent_Declare(
            Criterion
            GIT_REPOSITORY https://github.com/Snaipe/Criterion
            GIT_TAG        9c01cbe75002ad8640e0f411f453fbcd0567ff79 # v2.4.2
        )

        FetchContent_MakeAvailable(Criterion)
    endif()

    add_executable(tests ${SOURCE_FILES} ${TEST_SRC_FILES})
    target_include_directories(tests PRIVATE tests ${INCLUDE_DIRS} criterion)
    target_compile_definitions(tests PRIVATE MAYO_TESTS)

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
endif()

# Set the name of the executable
add_executable(my_lang src/main.c ${SOURCE_FILES})
target_include_directories(my_lang PRIVATE ${INCLUDE_DIRS})

# Set compiler flags for debug build
target_compile_options(my_lang PRIVATE -g -Wall -Wextra -Werror -Wmissing-prototypes -Wstrict-prototypes -pedantic)
