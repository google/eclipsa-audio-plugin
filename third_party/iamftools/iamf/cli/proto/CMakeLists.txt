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

include(FetchContent)

# Abseil first, Protobuf uses it
FetchContent_Declare(
  abseil
  GIT_REPOSITORY https://github.com/abseil/abseil-cpp.git
  GIT_TAG 20250512.1
)
FetchContent_MakeAvailable(abseil)

# Make protobuf use external abseil we just fetched
set(protobuf_ABSL_PROVIDER "package" CACHE STRING "" FORCE)
set(protobuf_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(protobuf_BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(protobuf_INSTALL OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
  protobuf
  GIT_REPOSITORY https://github.com/protocolbuffers/protobuf.git
  GIT_TAG v32.0
)
FetchContent_MakeAvailable(protobuf)

# Pull in the modern generator helpers provided by the Protobuf repo
include(${protobuf_SOURCE_DIR}/cmake/protobuf-generate.cmake)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)
# set(CMAKE_CXX_STANDARD 17)

# ---- Proto files (paths are relative to this dir) ----
set(PROTO_FILES
  arbitrary_obu.proto
  audio_element.proto
  audio_frame.proto
  codec_config.proto
  ia_sequence_header.proto
  mix_presentation.proto
  obu_header.proto
  output_audio_format.proto
  param_definitions.proto
  parameter_block.proto
  parameter_data.proto
  temporal_delimiter.proto
  test_vector_metadata.proto
  encoder_control_metadata.proto
  user_metadata.proto
)

# Build lib from generated sources; APPEND_PATH keeps iamf/cli/proto structure
add_library(proto-objects)
protobuf_generate(
  TARGET proto-objects
  LANGUAGE cpp
  PROTOS ${PROTO_FILES}
  IMPORT_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
  PROTOC_OUT_DIR ${CMAKE_CURRENT_BINARY_DIR}
  APPEND_PATH
)

target_link_libraries(proto-objects
  PUBLIC
    protobuf::libprotobuf
    absl::base
    absl::strings
    absl::core_headers
)
target_include_directories(proto-objects
  PUBLIC ${CMAKE_CURRENT_BINARY_DIR}
)