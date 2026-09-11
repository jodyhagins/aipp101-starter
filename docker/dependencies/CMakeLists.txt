cmake_minimum_required(VERSION 3.27)
project(wjh_workshop_dependencies LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(BUILD_SHARED_LIBS OFF)
set(FETCHCONTENT_TRY_FIND_PACKAGE_MODE NEVER)
include(../../cmake/DependencySources.cmake)

set(EXPECTED_BUILD_TESTS OFF)
set(EXPECTED_BUILD_PACKAGE OFF)
set(JSON_BuildTests OFF)
set(JSON_Install ON)
set(HTTPLIB_REQUIRE_OPENSSL ON)
set(HTTPLIB_COMPILE ON)
set(HTTPLIB_INSTALL ON)
set(HTTPLIB_TEST OFF)
set(BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(DOCTEST_WITH_TESTS OFF)
set(DOCTEST_WITH_MAIN_IN_STATIC_LIB OFF)
set(RC_ENABLE_TESTS OFF)
set(RC_ENABLE_EXAMPLES OFF)
set(RC_ENABLE_DOCTEST ON)

FetchContent_MakeAvailable(tl-expected nlohmann_json httplib dotenv DocTest rapidcheck)

# Record the source lock in the image so instructors can identify its contents.
install(FILES ../../cmake/DependencySources.cmake
    DESTINATION share/wjh-workshop)
file(SHA256 "${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/DependencySources.cmake" sources_sha)
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/WjhWorkshopDependenciesConfig.cmake"
    "set(WJH_WORKSHOP_DEPENDENCY_SOURCES_SHA256 ${sources_sha})\n")
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/WjhWorkshopDependenciesConfig.cmake"
    DESTINATION lib/cmake/WjhWorkshopDependencies)
