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
    message(STATUS "Linking prebuilt IAMF Tools from third_party/Windows")

    set(WINDOWS_LIB_DIR "${CMAKE_CURRENT_LIST_DIR}/lib/Windows/${BUILD_LIB_DIR}")

    set_target_properties(iamftools_dylib PROPERTIES
            IMPORTED_LOCATION "${WINDOWS_LIB_DIR}/iamf_tools.dll"
            IMPORTED_IMPLIB   "${WINDOWS_LIB_DIR}/iamf_tools.if.lib"
            IMPORTED_LOCATION_DEBUG   "${WINDOWS_LIB_DIR}/iamf_tools.dll"
            IMPORTED_LOCATION_RELEASE "${WINDOWS_LIB_DIR}/iamf_tools.dll"
            LINKER_LANGUAGE CXX
    )

    # Link the import lib so MSVC resolves symbols like IamfEncoderFactory::CreateFileGeneratingIamfEncoder
    target_link_libraries(iamftools INTERFACE
            "${WINDOWS_LIB_DIR}/iamf_tools.if.lib"
    )

    # Copy DLL next to the built plugin
    add_custom_target(copy_iamftools_dll ALL
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${WINDOWS_LIB_DIR}/iamf_tools.dll"
            "${CMAKE_BINARY_DIR}/iamf_tools.dll"
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
