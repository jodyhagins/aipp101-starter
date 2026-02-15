## ----------------------------------------------------------------------
## Copyright 2025 Jody Hagins
## Distributed under the MIT Software License
## See accompanying file LICENSE or copy at
## https://opensource.org/licenses/MIT
## ----------------------------------------------------------------------

# Always-needed dependencies

message(STATUS "Processing third-party Atlas...")
FetchContent_Declare(
        Atlas
        GIT_REPOSITORY /Users/jhagins/Dropbox/Programming/remotes/atlas.git
        GIT_TAG main
        SYSTEM
)
FetchContent_MakeAvailable(Atlas)

message(STATUS "Processing third-party TartanLlama/expected...")
set(EXPECTED_BUILD_TESTS OFF)
set(EXPECTED_BUILD_PACKAGE OFF)
set(EXPECTED_BUILD_PACKAGE_DEB OFF)
set(EXPECTED_BUILD_PACKAGE_RPM OFF)
FetchContent_Declare(
        tl-expected
        GIT_REPOSITORY https://github.com/TartanLlama/expected.git
        GIT_TAG master
        SYSTEM
)
FetchContent_MakeAvailable(tl-expected)

message(STATUS "Processing third-party nlohmann/json...")
set(JSON_BuildTests OFF)
set(JSON_Install OFF)
FetchContent_Declare(
        nlohmann_json
        GIT_REPOSITORY https://github.com/nlohmann/json.git
        GIT_TAG v3.11.3
        SYSTEM
)
FetchContent_MakeAvailable(nlohmann_json)

message(STATUS "Processing third-party cpp-httplib...")
set(HTTPLIB_REQUIRE_OPENSSL ON)
set(HTTPLIB_COMPILE OFF)
FetchContent_Declare(
        httplib
        GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
        GIT_TAG v0.18.3
        SYSTEM
)
FetchContent_MakeAvailable(httplib)

message(STATUS "Processing third-party laserpants/dotenv-cpp...")
set(BUILD_DOCS OFF CACHE INTERNAL "")
FetchContent_Declare(
        dotenv
        GIT_REPOSITORY https://github.com/laserpants/dotenv-cpp.git
        GIT_TAG master
        SYSTEM
)
FetchContent_MakeAvailable(dotenv)

# Test dependencies (conditional)
if (WJH_CHAT_BUILD_TESTS)
    string(REGEX REPLACE "(^| )-g([0-9]?)( |$)" "\\1-g3\\3" tmp "${CMAKE_CXX_FLAGS_DEBUG}")
    if (NOT "${CMAKE_CXX_FLAGS_DEBUG}" STREQUAL "${tmp}")
        message(STATUS "Changing CMAKE_CXX_FLAGS_DEBUG from '${CMAKE_CXX_FLAGS_DEBUG}' to '${tmp}'")
        set(CMAKE_CXX_FLAGS_DEBUG "${tmp}")
    endif ()

    message(STATUS "Processing third-party DocTest...")
    FetchContent_Declare(
            DocTest
            GIT_REPOSITORY https://github.com/jodyhagins/doctest.git
            GIT_TAG dev
            SYSTEM
    )
    FetchContent_MakeAvailable(DocTest)

    message(STATUS "Processing third-party RapidCheck...")
    set(RC_ENABLE_DOCTEST ON)
    FetchContent_Declare(
            rapidcheck
            GIT_REPOSITORY https://github.com/jodyhagins/rapidcheck.git
            GIT_TAG wjh-master
            SYSTEM
    )
    FetchContent_MakeAvailable(rapidcheck)
endif ()
