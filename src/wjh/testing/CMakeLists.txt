## ----------------------------------------------------------------------
## Copyright 2025 Jody Hagins
## Distributed under the MIT Software License
## See accompanying file LICENSE or copy at
## https://opensource.org/licenses/MIT
## ----------------------------------------------------------------------
add_library(wjh_chat_testing STATIC)
add_library(wjh::chat::testing ALIAS wjh_chat_testing)

target_sources(wjh_chat_testing
        PRIVATE
        MockClient.cpp

        PUBLIC
        MockClient.hpp
        doctest.hpp
)

target_link_libraries(wjh_chat_testing
        PUBLIC
        wjh::chat::client
)

target_include_directories(wjh_chat_testing
        PUBLIC
        "${PROJECT_SOURCE_DIR}/src/wjh"
        PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}")
