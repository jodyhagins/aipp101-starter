## ----------------------------------------------------------------------
## Copyright 2025 Jody Hagins
## Distributed under the MIT Software License
## See accompanying file LICENSE or copy at
## https://opensource.org/licenses/MIT
## ----------------------------------------------------------------------

# Main library target
add_library(wjh_chat STATIC)
add_library(wjh::chat ALIAS wjh_chat)

atlas_strong_types_from_file(
    INPUT ${CMAKE_CURRENT_SOURCE_DIR}/types.atlas
    OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/types_gen.hpp
    TARGET wjh_chat
)

target_sources(wjh_chat
        PRIVATE
        CommandLine.cpp
        Config.cpp
        ChatLoop.cpp

        PUBLIC
        ChatLoop.hpp
        CommandLine.hpp
        Config.hpp
        Result.hpp
        stdfmt.hpp
        json_convert.hpp
        types.hpp
        types_gen.hpp
)

target_link_libraries(wjh_chat
        PUBLIC
        tl::expected
        nlohmann_json::nlohmann_json

        PRIVATE
        wjh::chat::client
        wjh::chat::conversation
        dotenv
)

target_include_directories(wjh_chat
        PUBLIC
        "${PROJECT_SOURCE_DIR}/src")

# Component subdirectories
add_subdirectory(client)
add_subdirectory(conversation)

# Tests
if (WJH_CHAT_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif ()
