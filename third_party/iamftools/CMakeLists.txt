# Copyright 2025 Google LLC
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy at:
#      http://www.apache.org/licenses/LICENSE-2.0

add_subdirectory(iamf/cli/proto)

# Parse commit hash from README.md and generate header
set(README_FILE "${CMAKE_CURRENT_LIST_DIR}/README.md")
file(STRINGS "${README_FILE}" COMMIT_LINE REGEX "Commit: https://github.com/AOMediaCodec/iamf-tools/commit/([a-f0-9]+)")
string(REGEX REPLACE ".*commit/([a-f0-9]+).*" "GitHub/iamf-tools@\\1" COMMIT_HASH "${COMMIT_LINE}")

set(HEADER_FILE "${CMAKE_CURRENT_LIST_DIR}/iamf/include/iamf_tools/commit_hash.h")
set(TEMPLATE_FILE "${CMAKE_CURRENT_LIST_DIR}/iamf/include/iamf_tools/commit_hash.h.in")
configure_file(${TEMPLATE_FILE} ${HEADER_FILE} @ONLY)

# -------------------- IAMF TOOLS --------------------
add_library(iamftools INTERFACE)
target_compile_options(iamftools INTERFACE "-w")
target_include_directories(iamftools INTERFACE
        "${CMAKE_CURRENT_SOURCE_DIR}"
        "${CMAKE_CURRENT_SOURCE_DIR}/iamf/include/iamf_tools"
)

# Cross-platform dynamic library setup
add_library(iamftools_dylib SHARED IMPORTED)

if(WIN32)
    # Platform-specific setup for Windows
    set(IAMF_PLATFORM "Windows")

    # Set paths for each configuration
    set(IAMF_TOOLS_DEBUG_DLL "${CMAKE_CURRENT_LIST_DIR}/lib/${IAMF_PLATFORM}/Debug/iamf_tools.dll")
    set(IAMF_TOOLS_DEBUG_LIB "${CMAKE_CURRENT_LIST_DIR}/lib/${IAMF_PLATFORM}/Debug/iamf_tools.if.lib")
    set(IAMF_TOOLS_RELEASE_DLL "${CMAKE_CURRENT_LIST_DIR}/lib/${IAMF_PLATFORM}/Release/iamf_tools.dll")
    set(IAMF_TOOLS_RELEASE_LIB "${CMAKE_CURRENT_LIST_DIR}/lib/${IAMF_PLATFORM}/Release/iamf_tools.if.lib")

    # Validate that the libraries exist
    if(NOT EXISTS "${IAMF_TOOLS_DEBUG_DLL}")
        message(FATAL_ERROR "IAMF Tools Debug DLL not found at: ${IAMF_TOOLS_DEBUG_DLL}")
    endif()
    if(NOT EXISTS "${IAMF_TOOLS_DEBUG_LIB}")
        message(FATAL_ERROR "IAMF Tools Debug LIB not found at: ${IAMF_TOOLS_DEBUG_LIB}")
    endif()
    if(NOT EXISTS "${IAMF_TOOLS_RELEASE_DLL}")
        message(FATAL_ERROR "IAMF Tools Release DLL not found at: ${IAMF_TOOLS_RELEASE_DLL}")
    endif()
    if(NOT EXISTS "${IAMF_TOOLS_RELEASE_LIB}")
        message(FATAL_ERROR "IAMF Tools Release LIB not found at: ${IAMF_TOOLS_RELEASE_LIB}")
    endif()

    message(STATUS "IAMF Tools Debug DLL: ${IAMF_TOOLS_DEBUG_DLL}")
    message(STATUS "IAMF Tools Release DLL: ${IAMF_TOOLS_RELEASE_DLL}")

    set_target_properties(iamftools_dylib PROPERTIES
            IMPORTED_LOCATION "${IAMF_TOOLS_RELEASE_DLL}"
            IMPORTED_LOCATION_DEBUG "${IAMF_TOOLS_DEBUG_DLL}"
            IMPORTED_LOCATION_RELEASE "${IAMF_TOOLS_RELEASE_DLL}"
            IMPORTED_IMPLIB "${IAMF_TOOLS_RELEASE_LIB}"
            IMPORTED_IMPLIB_DEBUG "${IAMF_TOOLS_DEBUG_LIB}"
            IMPORTED_IMPLIB_RELEASE "${IAMF_TOOLS_RELEASE_LIB}"
            LINKER_LANGUAGE CXX
    )

    # Copy DLL to build directory (configuration-aware using generator expressions)
    add_custom_target(copy_iamftools_dll ALL
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<IF:$<CONFIG:Debug>,${IAMF_TOOLS_DEBUG_DLL},${IAMF_TOOLS_RELEASE_DLL}>"
            "${CMAKE_BINARY_DIR}/$<CONFIG>/iamf_tools.dll"
            COMMENT "Copying IAMF Tools DLL to build directory"
    )

else()
    # macOS: dylib
    set_target_properties(iamftools_dylib PROPERTIES
            IMPORTED_LOCATION "${CMAKE_CURRENT_LIST_DIR}/lib/libiamf_tools.dylib"
            IMPORTED_SONAME "libiamf_tools.dylib"
            LINKER_LANGUAGE CXX
    )
endif()

target_link_libraries(iamftools INTERFACE
        iamftools_dylib
        proto-objects
)
