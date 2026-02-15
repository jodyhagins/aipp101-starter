## ----------------------------------------------------------------------
## Copyright 2025 Jody Hagins
## Distributed under the MIT Software License
## See accompanying file LICENSE or copy at
## https://opensource.org/licenses/MIT
## ----------------------------------------------------------------------
cmake_minimum_required(VERSION 3.27)

project(wjh_chat
        VERSION 0.1.0
        DESCRIPTION "AI++ 101 Chat Application"
        LANGUAGES CXX)

list(APPEND CMAKE_PREFIX_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")
list(PREPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

include(Setup)
if (CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    option(WJH_CHAT_BUILD_TESTS "whether or not to build tests" ON)
    option(WJH_CHAT_SANITIZE "whether or not to use address sanitizer" ON)
endif ()

find_package(Threads REQUIRED)
include(FetchContent)
include(CTest)
include(CompileOptions)
if (WJH_CHAT_SANITIZE)
    add_compile_options(-fsanitize=address -g)
    add_link_options(-fsanitize=address)
endif ()
include(ThirdParty)
include(AtlasWithFormat)

create_wjh_compile_options_target()
link_libraries(wjh_compile_options)

add_subdirectory(src)
