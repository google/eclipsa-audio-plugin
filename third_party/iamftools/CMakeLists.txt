# Copyright 2025 Google LLC
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

add_subdirectory(iamf/cli/proto)
# --------------- Commit Hash Handling ---------------
# Parse the commit hash from the README.md file and generate a header file with it
# Set the path to the README.md file
set(README_FILE "${CMAKE_CURRENT_LIST_DIR}/README.md")

file(STRINGS "${README_FILE}" COMMIT_LINE REGEX "Commit: https://github.com/AOMediaCodec/iamf-tools/commit/([a-f0-9]+)")
string(REGEX REPLACE ".*commit/([a-f0-9]+).*" "GitHub/iamf-tools@\\1" COMMIT_HASH "${COMMIT_LINE}")

# Set the path to the header and template file
set(HEADER_FILE "${CMAKE_CURRENT_LIST_DIR}/iamf/include/iamf_tools/commit_hash.h")
set(TEMPLATE_FILE "${CMAKE_CURRENT_LIST_DIR}/iamf/include/iamf_tools/commit_hash.h.in")

# Configure the header file
configure_file(${TEMPLATE_FILE} ${HEADER_FILE} @ONLY)

# -------------------- IAMF TOOLS --------------------
add_library(iamftools INTERFACE)
target_compile_options(iamftools INTERFACE "-w")
target_include_directories(iamftools INTERFACE
  "${CMAKE_CURRENT_SOURCE_DIR}"
  "${CMAKE_CURRENT_SOURCE_DIR}/iamf/include/iamf_tools"
)

add_library(iamftools_dylib SHARED IMPORTED)
set_target_properties(iamftools_dylib PROPERTIES
  IMPORTED_LOCATION "${CMAKE_CURRENT_LIST_DIR}/lib/libiamf_tools.dylib"
  IMPORTED_SONAME "libiamf_tools.dylib"
  LINKER_LANGUAGE CXX
)

target_link_libraries(iamftools
  INTERFACE
    iamftools_dylib
    proto-objects
)
