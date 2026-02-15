## ----------------------------------------------------------------------
## Copyright 2025 Jody Hagins
## Distributed under the MIT Software License
## See accompanying file LICENSE or copy at
## https://opensource.org/licenses/MIT
## ----------------------------------------------------------------------

add_library(wjh_chat_client STATIC)
add_library(wjh::chat::client ALIAS wjh_chat_client)

atlas_strong_types_from_file(
    INPUT ${CMAKE_CURRENT_SOURCE_DIR}/types.atlas
    OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/types_gen.hpp
    TARGET wjh_chat_client
)

target_sources(wjh_chat_client
        PRIVATE
        HttpClient.cpp
        OpenRouterClient.cpp
        IClient.cpp

        PUBLIC
        HttpClient.hpp
        OpenRouterClient.hpp
        IClient.hpp
        types.hpp
        types_gen.hpp
)

target_link_libraries(wjh_chat_client
        PUBLIC
        tl::expected
        nlohmann_json::nlohmann_json
        httplib::httplib
        wjh::chat::conversation
)

target_include_directories(wjh_chat_client
        PUBLIC
        "${PROJECT_SOURCE_DIR}/src")
