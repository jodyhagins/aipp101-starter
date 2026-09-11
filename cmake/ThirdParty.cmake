## ----------------------------------------------------------------------
## Copyright 2025 Jody Hagins
## Distributed under the MIT Software License
## See accompanying file LICENSE or copy at
## https://opensource.org/licenses/MIT
## ----------------------------------------------------------------------

include(DependencySources)

option(WJH_CHAT_REQUIRE_INSTALLED_DEPS
    "Require installed dependencies; never download or build third-party code" OFF)

if (WJH_CHAT_REQUIRE_INSTALLED_DEPS)
    find_package(WjhWorkshopDependencies CONFIG QUIET)
    if (WjhWorkshopDependencies_FOUND)
        file(SHA256 "${CMAKE_CURRENT_LIST_DIR}/DependencySources.cmake" sources_sha)
        if (NOT sources_sha STREQUAL WJH_WORKSHOP_DEPENDENCY_SOURCES_SHA256)
            message(FATAL_ERROR
                "The workshop image's dependency pins differ from this checkout. "
                "Pull the updated workshop image supplied by the instructor.")
        endif ()
    endif ()
endif ()

# A macro keeps imported targets visible to all of the project's subdirectories.
macro(wjh_find_or_fetch name package target)
    if (NOT TARGET ${target})
        find_package(${package} ${ARGN} CONFIG QUIET)
        if (${package}_FOUND AND TARGET ${target})
            message(STATUS "Using installed ${package}: ${${package}_DIR}")
        elseif (WJH_CHAT_REQUIRE_INSTALLED_DEPS)
            message(FATAL_ERROR
                "Required installed dependency '${package}' is missing or incompatible. "
                "Use the workshop image matching this checkout, or set "
                "CMAKE_PREFIX_PATH to your dependency installation. "
                "WJH_CHAT_REQUIRE_INSTALLED_DEPS prevents downloading a fallback.")
        elseif (${package}_FOUND OR TARGET ${target})
            message(FATAL_ERROR
                "Installed package '${package}' does not provide a usable '${target}'. "
                "Reinstall it with the required integrations enabled, or disable its "
                "discovery in a fresh build directory to use the source fallback.")
        else ()
            message(STATUS "Building ${name} from its declared source revision...")
            FetchContent_MakeAvailable(${name})
        endif ()
    endif ()
endmacro()

wjh_find_or_fetch(Atlas Atlas Atlas::atlas)

set(EXPECTED_BUILD_TESTS OFF)
set(EXPECTED_BUILD_PACKAGE OFF)
wjh_find_or_fetch(tl-expected tl-expected tl::expected 1.3.1 EXACT)

set(JSON_BuildTests OFF)
set(JSON_Install OFF)
wjh_find_or_fetch(nlohmann_json nlohmann_json nlohmann_json::nlohmann_json 3.11.3 EXACT)

set(HTTPLIB_REQUIRE_OPENSSL ON)
set(HTTPLIB_COMPILE OFF)
wjh_find_or_fetch(httplib httplib httplib::httplib 0.18.3 EXACT COMPONENTS OpenSSL)

set(BUILD_DOCS OFF CACHE INTERNAL "")
wjh_find_or_fetch(dotenv laserpants_dotenv laserpants::dotenv)
if (TARGET laserpants::dotenv AND NOT TARGET dotenv)
    add_library(dotenv ALIAS laserpants::dotenv)
endif ()

if (WJH_CHAT_BUILD_TESTS)
    string(REGEX REPLACE "(^| )-g([0-9]?)( |$)" "\\1-g3\\3" tmp "${CMAKE_CXX_FLAGS_DEBUG}")
    if (NOT "${CMAKE_CXX_FLAGS_DEBUG}" STREQUAL "${tmp}")
        set(CMAKE_CXX_FLAGS_DEBUG "${tmp}")
    endif ()

    wjh_find_or_fetch(DocTest doctest doctest::doctest)
    if (NOT TARGET doctest)
        add_library(doctest ALIAS doctest::doctest)
    endif ()

    set(RC_ENABLE_DOCTEST ON)
    wjh_find_or_fetch(rapidcheck rapidcheck rapidcheck_doctest)
endif ()
