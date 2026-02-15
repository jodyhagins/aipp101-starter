## ----------------------------------------------------------------------
## Copyright 2025 Jody Hagins
## Distributed under the MIT Software License
## See accompanying file LICENSE or copy at
## https://opensource.org/licenses/MIT
## ----------------------------------------------------------------------

macro(_atlas_get_executable)
    if(TARGET Atlas::atlas)
        set(_ATLAS_EXE $<TARGET_FILE:Atlas::atlas>)
    elseif(DEFINED Atlas_EXECUTABLE)
        set(_ATLAS_EXE ${Atlas_EXECUTABLE})
    else()
        message(FATAL_ERROR "Atlas executable not found")
    endif()
endmacro()

macro(_atlas_build_commands OUTPUT_FILE)
    set(_CMDS COMMAND ${_ATLAS_EXE} ${_ATLAS_ARGS})
endmacro()

function(_atlas_write_if_changed filepath content)
    if(EXISTS "${filepath}")
        file(READ "${filepath}" _existing)
    else()
        set(_existing "")
    endif()
    if(NOT "${_existing}" STREQUAL "${content}")
        file(WRITE "${filepath}" "${content}")
    endif()
endfunction()

function(_atlas_create_target target_name output_file parent_target)
    add_custom_target(${target_name} DEPENDS ${output_file})
    if(parent_target)
        if(TARGET ${parent_target})
            add_dependencies(${parent_target} ${target_name})
            target_sources(${parent_target} PUBLIC ${output_file})
        else()
            message(WARNING "Target '${parent_target}' does not exist")
        endif()
    endif()
endfunction()

function(atlas_strong_types_inline)
    cmake_parse_arguments(ARG "" "OUTPUT;TARGET" "CONTENT" ${ARGN})
    if(NOT ARG_OUTPUT)
        message(FATAL_ERROR "atlas_strong_types_inline: OUTPUT is required")
    endif()
    if(NOT ARG_CONTENT)
        message(FATAL_ERROR "atlas_strong_types_inline: CONTENT is required")
    endif()

    get_filename_component(_name ${ARG_OUTPUT} NAME_WE)
    set(_input "${CMAKE_CURRENT_BINARY_DIR}/.atlas_inline_${_name}.atlas")
    _atlas_write_if_changed("${_input}" "${ARG_CONTENT}")

    _atlas_get_executable()
    set(_ATLAS_ARGS --input=${_input} --output=${ARG_OUTPUT} --auto-hash=yes --auto-ostream=yes --auto-istream=yes --auto-format=yes)
    _atlas_build_commands(${ARG_OUTPUT})

    add_custom_command(
        OUTPUT ${ARG_OUTPUT}
        ${_CMDS}
        DEPENDS ${_ATLAS_EXE} ${_input}
        COMMENT "Generating ${_name} with Atlas"
        VERBATIM)

    if(ARG_TARGET)
        set(_target "generate_${ARG_TARGET}_${_name}")
    else()
        set(_target "generate_${_name}")
    endif()
    _atlas_create_target(${_target} ${ARG_OUTPUT} "${ARG_TARGET}")
endfunction()

function(atlas_strong_types_from_file)
    cmake_parse_arguments(ARG "" "INPUT;OUTPUT;TARGET" "" ${ARGN})
    if(NOT ARG_INPUT)
        message(FATAL_ERROR "atlas_strong_types_from_file: INPUT is required")
    endif()
    if(NOT ARG_OUTPUT)
        message(FATAL_ERROR "atlas_strong_types_from_file: OUTPUT is required")
    endif()

    if(NOT IS_ABSOLUTE "${ARG_INPUT}")
        set(ARG_INPUT "${CMAKE_CURRENT_SOURCE_DIR}/${ARG_INPUT}")
    endif()

    get_filename_component(_name ${ARG_OUTPUT} NAME_WE)
    _atlas_get_executable()
    set(_ATLAS_ARGS --input=${ARG_INPUT} --output=${ARG_OUTPUT} --auto-hash=yes --auto-ostream=yes --auto-istream=yes --auto-format=yes)
    _atlas_build_commands(${ARG_OUTPUT})

    add_custom_command(
        OUTPUT ${ARG_OUTPUT}
        ${_CMDS}
        DEPENDS ${_ATLAS_EXE} ${ARG_INPUT}
        COMMENT "Generating ${_name} with Atlas"
        VERBATIM)

    if(ARG_TARGET)
        set(_target "generate_${ARG_TARGET}_${_name}")
    else()
        set(_target "generate_${_name}")
    endif()
    _atlas_create_target(${_target} ${ARG_OUTPUT} "${ARG_TARGET}")
endfunction()

# Generate interactions from file with formatting
function(atlas_interactions_from_file)
    cmake_parse_arguments(ARG "" "INPUT;OUTPUT;TARGET" "" ${ARGN})
    if(NOT ARG_INPUT)
        message(FATAL_ERROR "atlas_interactions_from_file: INPUT is required")
    endif()
    if(NOT ARG_OUTPUT)
        message(FATAL_ERROR "atlas_interactions_from_file: OUTPUT is required")
    endif()

    if(NOT IS_ABSOLUTE "${ARG_INPUT}")
        set(ARG_INPUT "${CMAKE_CURRENT_SOURCE_DIR}/${ARG_INPUT}")
    endif()

    get_filename_component(_name ${ARG_OUTPUT} NAME_WE)
    _atlas_get_executable()
    set(_ATLAS_ARGS --input=${ARG_INPUT} --interactions=true --output=${ARG_OUTPUT})
    _atlas_build_commands(${ARG_OUTPUT})

    add_custom_command(
        OUTPUT ${ARG_OUTPUT}
        ${_CMDS}
        DEPENDS ${_ATLAS_EXE} ${ARG_INPUT}
        COMMENT "Generating ${_name} with Atlas"
        VERBATIM)

    # Create target
    if(ARG_TARGET)
        set(_target "generate_${ARG_TARGET}_${_name}")
    else()
        set(_target "generate_${_name}")
    endif()
    _atlas_create_target(${_target} ${ARG_OUTPUT} "${ARG_TARGET}")
endfunction()
