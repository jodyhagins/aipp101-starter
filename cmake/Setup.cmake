## ----------------------------------------------------------------------
## Copyright 2025 Jody Hagins
## Distributed under the MIT Software License
## See accompanying file LICENSE or copy at
## https://opensource.org/licenses/MIT
## ----------------------------------------------------------------------
if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif ()

if (NOT CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD "20")
endif ()

option(WJH_DETERMINE_CACHE_LINE_SIZE
        "Compute value for WJH_DETERMINE_CACHE_LINE_SIZE if not set"
        ON)

if (NOT WJH_CACHE_LINE_SIZE AND WJH_DETERMINE_CACHE_LINE_SIZE)
    if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin" OR ${CMAKE_SYSTEM_NAME} MATCHES "Linux")
        execute_process(
                COMMAND ${CMAKE_SOURCE_DIR}/scripts/hw-cache-line-size ${CMAKE_SYSTEM_NAME}
                OUTPUT_VARIABLE WJH_CACHE_LINE_SIZE
                OUTPUT_STRIP_TRAILING_WHITESPACE
        )
    else ()
        message(FATAL_ERROR "Unrecognized CMAKE_SYSTEM_NAME=${CMAKE_SYSTEM_NAME}")
    endif ()
    message(STATUS "Setting WJH_CACHE_LINE_SIZE to computed value of "
            ${WJH_CACHE_LINE_SIZE}
    )
endif ()
if (NOT WJH_CACHE_LINE_SIZE)
    message(FATAL_ERROR "WJH_CACHE_LINE_SIZE is not set.")
endif ()
