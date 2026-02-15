## ----------------------------------------------------------------------
## Copyright 2025 Jody Hagins
## Distributed under the MIT Software License
## See accompanying file LICENSE or copy at
## https://opensource.org/licenses/MIT
## ----------------------------------------------------------------------

add_library(wjh_chat_conversation STATIC)
add_library(wjh::chat::conversation ALIAS wjh_chat_conversation)

atlas_strong_types_from_file(
    INPUT ${CMAKE_CURRENT_SOURCE_DIR}/types.atlas
    OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/types_gen.hpp
    TARGET wjh_chat_conversation
)

target_sources(wjh_chat_conversation
        PRIVATE
        Message.cpp
        Conversation.cpp

        PUBLIC
        Message.hpp
        Conversation.hpp
        types.hpp
        types_gen.hpp
)

target_link_libraries(wjh_chat_conversation
        PUBLIC
        tl::expected
        nlohmann_json::nlohmann_json
)

target_include_directories(wjh_chat_conversation
        PUBLIC
        "${PROJECT_SOURCE_DIR}/src")
