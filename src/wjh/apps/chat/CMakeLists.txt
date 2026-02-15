## ----------------------------------------------------------------------
## Copyright 2025 Jody Hagins
## Distributed under the MIT Software License
## See accompanying file LICENSE or copy at
## https://opensource.org/licenses/MIT
## ----------------------------------------------------------------------
add_executable(chat_app main.cpp)

target_link_libraries(chat_app
        PRIVATE
        wjh::chat
        wjh::chat::client
        wjh::chat::conversation
)
