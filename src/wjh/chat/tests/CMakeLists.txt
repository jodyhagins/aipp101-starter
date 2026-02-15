## ----------------------------------------------------------------------
## Copyright 2025 Jody Hagins
## Distributed under the MIT Software License
## See accompanying file LICENSE or copy at
## https://opensource.org/licenses/MIT
## ----------------------------------------------------------------------

add_executable(chat_ut
        main.cpp
        Result_ut.cpp
        Message_ut.cpp
        Conversation_ut.cpp
        CommandLine_ut.cpp
        Config_ut.cpp
        OpenRouterClient_ut.cpp
        ChatLoop_ut.cpp
)

target_link_libraries(chat_ut
        PRIVATE
        wjh::chat
        wjh::chat::testing
        Threads::Threads
        rapidcheck_doctest
        doctest
)

add_test(NAME "Chat Tests" COMMAND chat_ut)
