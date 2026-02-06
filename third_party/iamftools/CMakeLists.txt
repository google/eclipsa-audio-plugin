# Copyright 2025 Google LLC
#
# Licensed under the Apache License, Version 2.0 (the "License");
# You may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.

add_subdirectory(iamf/cli/proto)

# Parse commit hash from README.md and generate header
set(README_FILE "${CMAKE_CURRENT_LIST_DIR}/README.md")
file(STRINGS "${README_FILE}" COMMIT_LINE REGEX "Commit: https://github.com/AOMediaCodec/iamf-tools/commit/([a-f0-9]+)")
string(REGEX REPLACE ".*commit/([a-f0-9]+).*" "GitHub/iamf-tools@\\1" COMMIT_HASH "${COMMIT_LINE}")

set(HEADER_FILE "${CMAKE_CURRENT_LIST_DIR}/iamf/include/iamf_tools/commit_hash.h")
set(TEMPLATE_FILE "${CMAKE_CURRENT_LIST_DIR}/iamf/include/iamf_tools/commit_hash.h.in")
configure_file(${TEMPLATE_FILE} ${HEADER_FILE} @ONLY)

add_library(iamftools INTERFACE)
target_compile_options(iamftools INTERFACE "-w")
target_include_directories(iamftools INTERFACE
        "${CMAKE_CURRENT_SOURCE_DIR}"
        "${CMAKE_CURRENT_SOURCE_DIR}/iamf/include/iamf_tools"
)

target_link_libraries(iamftools INTERFACE iamftools_impl proto-objects)