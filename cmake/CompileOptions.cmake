## ----------------------------------------------------------------------
## Copyright 2025 Jody Hagins
## Distributed under the MIT Software License
## See accompanying file LICENSE or copy at
## https://opensource.org/licenses/MIT
## ----------------------------------------------------------------------

include(CheckCXXCompilerFlag)

function(check_cacheline_alignment)
    message(STATUS "Checking compiler cacheline assumptions...")

    execute_process(
            COMMAND ${CMAKE_CXX_COMPILER} -dM -E -
            INPUT_FILE /dev/null
            OUTPUT_VARIABLE MACRO_DEFS
            OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    string(REGEX MATCH "#define __GCC_DESTRUCTIVE_SIZE ([0-9]+)" _match_destructive "${MACRO_DEFS}")
    set(DESTRUCTIVE_SIZE "${CMAKE_MATCH_1}")
    string(REGEX MATCH "#define __GCC_CONSTRUCTIVE_SIZE ([0-9]+)" _match_constructive "${MACRO_DEFS}")
    set(CONSTRUCTIVE_SIZE "${CMAKE_MATCH_1}")

    if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin" OR ${CMAKE_SYSTEM_NAME} MATCHES "Linux")
        execute_process(
                COMMAND ${CMAKE_SOURCE_DIR}/scripts/hw-cache-line-size ${CMAKE_SYSTEM_NAME}
                OUTPUT_VARIABLE REAL_CACHELINE_SIZE
                OUTPUT_STRIP_TRAILING_WHITESPACE
        )
    else ()
        message(FATAL_ERROR "Unrecognized CMAKE_SYSTEM_NAME=${CMAKE_SYSTEM_NAME}")
    endif ()

    if (NOT DESTRUCTIVE_SIZE STREQUAL REAL_CACHELINE_SIZE OR NOT CONSTRUCTIVE_SIZE STREQUAL REAL_CACHELINE_SIZE)
        message(WARNING "Compiler cacheline size macros don't match hardware: "
                "__GCC_DESTRUCTIVE_SIZE=${DESTRUCTIVE_SIZE}"
                ", __GCC_CONSTRUCTIVE_SIZE=${CONSTRUCTIVE_SIZE}"
                ", REAL_CACHELINE_SIZE=${REAL_CACHELINE_SIZE}"
        )

        if (WJH_FORCE_CHANGE_GCC_CACHE_SIZES)
            message(WARNING "Adding global compiler definitions: "
                    "__GCC_DESTRUCTIVE_SIZE=${REAL_CACHELINE_SIZE}, and"
                    "__GCC_CONSTRUCTIVE_SIZE=${REAL_CACHELINE_SIZE}"
            )
            add_compile_definitions(
                    __GCC_DESTRUCTIVE_SIZE=${REAL_CACHELINE_SIZE}
                    __GCC_CONSTRUCTIVE_SIZE=${REAL_CACHELINE_SIZE}
            )
        endif ()
    endif ()

    if (NOT ${WJH_CACHE_LINE_SIZE} EQUAL ${REAL_CACHELINE_SIZE})
        message(FATAL_ERROR "WJH_CACHE_LINE_SIZE=${WJH_CACHE_LINE_SIZE}"
                " does not match REAL_CACHELINE_SIZE=${REAL_CACHELINE_SIZE}"
        )
    endif ()
endfunction()

function(create_wjh_compile_options_target)
    add_library(wjh_compile_options INTERFACE)

    set(COMMON_WARNINGS
            -Wall
            -Wextra
            -pedantic
            -Wcast-align
            -Wcast-qual
            -Wconversion
            -Wformat=2
            -Wnon-virtual-dtor
            -Wold-style-cast
            -Woverloaded-virtual
            -Wsign-conversion
            -Wshadow
            -Wswitch-enum
            -Wundef
            -Wunused
    )
    set(COMMON_NO_WARNINGS)

    set(GCC_WARNINGS
            -Wdisabled-optimization
            -Wlogical-op
            -Wsign-promo
            -Wredundant-decls
            -Wdouble-promotion
            -Wdate-time
            -Wsuggest-final-methods
            -Wsuggest-final-types
            -Wsuggest-override
            -Wduplicated-cond
            -Wmisleading-indentation
            -Wnull-dereference
            -Wduplicated-branches
            -Wextra-semi
            -Wunsafe-loop-optimizations
            -Warith-conversion
            -Wredundant-tags
    )
    set(GCC_NO_WARNINGS
            -Wno-multiple-inheritance
            -Wno-switch-default
            -Wno-useless-cast
    )

    set(CLANG_WARNINGS
            -Weverything
    )
    set(CLANG_NO_WARNINGS
            -Wno-c++20-compat
            -Wno-c++98-compat
            -Wno-c++98-compat-pedantic
            -Wno-padded
            -Wno-dollar-in-identifier-extension
            -Wno-comma
            -Wno-switch-default
            -Wno-unevaluated-expression
            -Wno-defaulted-function-deleted
    )

    set(WARNINGS ${COMMON_WARNINGS})
    if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        list(APPEND WARNINGS ${GCC_WARNINGS} ${GCC_NO_WARNINGS})
    elseif (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        list(APPEND WARNINGS ${CLANG_WARNINGS} ${CLANG_NO_WARNINGS})
        if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
            list(APPEND WARNINGS "-Wno-poison-system-directories")
        endif ()
    endif ()

    list(APPEND WARNINGS ${COMMON_NO_WARNINGS})
    LIST(REMOVE_DUPLICATES WARNINGS)
    target_compile_options(wjh_compile_options INTERFACE -Werror ${WARNINGS})

    check_cacheline_alignment()
    if (${WJH_CACHE_LINE_SIZE})
        target_compile_definitions(wjh_compile_options INTERFACE
                WJH_CACHE_LINE_SIZE=${WJH_CACHE_LINE_SIZE})
    endif ()

endfunction()
