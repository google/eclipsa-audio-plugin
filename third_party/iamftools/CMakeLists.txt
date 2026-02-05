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

if (DEFINED IAMF_TOOLS_DYLIB)
    # macOS: single dylib
    set_target_properties(iamftools_dylib PROPERTIES
            IMPORTED_LOCATION "${IAMF_TOOLS_DYLIB}"
            IMPORTED_SONAME "libiamf_tools.dylib"
            LINKER_LANGUAGE CXX
    )
else ()
    # Windows: dll + lib with debug/release
    foreach (_file ${IAMF_TOOLS_DEBUG_DLL} ${IAMF_TOOLS_DEBUG_LIB} ${IAMF_TOOLS_RELEASE_DLL} ${IAMF_TOOLS_RELEASE_LIB})
        if (NOT EXISTS "${_file}")
            message(FATAL_ERROR "IAMF Tools file not found: ${_file}")
        endif ()
    endforeach ()

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
endif ()

if (IAMF_TOOLS_COPY_DLL)
    add_custom_target(copy_iamftools_dll ALL
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<IF:$<CONFIG:Debug>,${IAMF_TOOLS_DEBUG_DLL},${IAMF_TOOLS_RELEASE_DLL}>"
            "${CMAKE_BINARY_DIR}/$<CONFIG>/iamf_tools.dll"
            COMMENT "Copying IAMF Tools DLL to build directory"
    )
endif ()

target_link_libraries(iamftools INTERFACE
        iamftools_dylib
        proto-objects
)