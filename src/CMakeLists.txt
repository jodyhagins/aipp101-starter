## ----------------------------------------------------------------------
## Copyright 2025 Jody Hagins
## Distributed under the MIT Software License
## See accompanying file LICENSE or copy at
## https://opensource.org/licenses/MIT
## ----------------------------------------------------------------------
add_subdirectory(wjh/chat)
add_subdirectory(wjh/apps)
if (WJH_CHAT_BUILD_TESTS)
    add_subdirectory(wjh/testing)
endif ()
