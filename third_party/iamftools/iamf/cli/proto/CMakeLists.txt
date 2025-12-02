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

cmake_minimum_required(VERSION 3.16)
project(iamf_proto LANGUAGES CXX)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# ---- Windows runtime consistency ----
if(WIN32)
    set(CMAKE_MSVC_RUNTIME_LIBRARY
            "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL"
            CACHE STRING "" FORCE)

    set(ABSL_USE_STATIC_MSVC_RUNTIME_LIBRARY OFF CACHE BOOL "" FORCE)
    set(protobuf_MSVC_STATIC_RUNTIME OFF CACHE BOOL "" FORCE)
    set(protobuf_ABSL_PROVIDER "module" CACHE STRING "" FORCE)
    set(protobuf_DOWNLOAD_ABSEIL ON CACHE BOOL "" FORCE)
else()
    # On mac we need to manually fetch abseil
    # Unsure why protobuf doesn't automatically fetch it here, should investigate later
    FetchContent_Declare(
            abseil
            GIT_REPOSITORY https://github.com/abseil/abseil-cpp.git
            GIT_TAG 20250512.1
    )
    FetchContent_MakeAvailable(abseil)
    set(protobuf_DOWNLOAD_ABSEIL OFF CACHE BOOL "" FORCE)
    set(protobuf_ABSL_PROVIDER "package" CACHE STRING "" FORCE)
endif()

# ---- Dependencies ----
include(FetchContent)

message(STATUS "Fetching Protobuf (Abseil handled internally)...")

set(PROTOBUF_VERSION v32.0)

# Tell protobuf to fetch Abseil internally if not found
set(protobuf_ABSL_PROVIDER "module" CACHE STRING "" FORCE)
set(protobuf_DOWNLOAD_ABSEIL OFF CACHE BOOL "" FORCE)

set(protobuf_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(protobuf_BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(protobuf_INSTALL OFF CACHE BOOL "" FORCE)
set(protobuf_FORCE_FETCH_DEPENDENCIES OFF CACHE BOOL "" FORCE)

# IMPORTANT: Disable local-only restriction (this was blocking Abseil fetch)
set(protobuf_LOCAL_DEPENDENCIES_ONLY OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
        protobuf
        GIT_REPOSITORY https://github.com/protocolbuffers/protobuf.git
        GIT_TAG ${PROTOBUF_VERSION}
)
FetchContent_MakeAvailable(protobuf)

# Include protobuf CMake helpers
include("${protobuf_SOURCE_DIR}/cmake/protobuf-generate.cmake")

# ---- Proto files ----
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

# ---- Build generated sources ----
add_library(proto-objects)
protobuf_generate(
        TARGET proto-objects
        LANGUAGE cpp
        PROTOS ${PROTO_FILES}
        IMPORT_DIRS .
        PROTOC_OUT_DIR ${CMAKE_CURRENT_BINARY_DIR}
        APPEND_PATH
)

# ---- Link & include ----
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
