# Shared source pins for the development fallback and workshop image.
# Update these deliberately, then build and distribute a new workshop image.
include(FetchContent)

FetchContent_Declare(Atlas
    GIT_REPOSITORY https://github.com/jodyhagins/atlas.git
    GIT_TAG main
    SYSTEM)

FetchContent_Declare(tl-expected
    GIT_REPOSITORY https://github.com/TartanLlama/expected.git
    GIT_TAG 1770e3559f2f6ea4a5fb4f577ad22aeb30fbd8e4 # master, 1.3.1
    GIT_SUBMODULES ""
    SYSTEM)

FetchContent_Declare(nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG 9cca280a4d0ccf0c08f47a99aa71d1b0e52f8d03 # v3.11.3
    GIT_SUBMODULES ""
    SYSTEM)

FetchContent_Declare(httplib
    GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
    GIT_TAG a7bc00e3307fecdb4d67545e93be7b88cfb1e186 # v0.18.3
    GIT_SUBMODULES ""
    SYSTEM)

FetchContent_Declare(dotenv
    GIT_REPOSITORY https://github.com/laserpants/dotenv-cpp.git
    GIT_TAG 9275210b8abf1a551fb81e0ba45866286d764acd # master, 0.9.3
    GIT_SUBMODULES ""
    SYSTEM)

FetchContent_Declare(DocTest
    GIT_REPOSITORY https://github.com/jodyhagins/doctest.git
    GIT_TAG 75dfbdafddfb5356070aa8f1325d656971d9ed9f # dev fork
    GIT_SUBMODULES ""
    SYSTEM)

FetchContent_Declare(rapidcheck
    GIT_REPOSITORY https://github.com/jodyhagins/rapidcheck.git
    GIT_TAG e879d1872f54c42a85f834b8b42c415127e07ae8 # wjh-master fork
    GIT_SUBMODULES ""
    SYSTEM)
