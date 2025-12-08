# Copyright 2024 Google LLC
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

message(STATUS "Fetching LibIAMF")

include(FetchContent)

FetchContent_Declare(
        libiamf
        URL https://github.com/AOMediaCodec/libiamf/archive/48b8b5dc06971e14826d114fd032dab285f4c944.zip
)

set(CODEC_CAP ON CACHE BOOL "Codec capability check" FORCE)
set(MULTICHANNEL_BINAURALIZER OFF CACHE BOOL "Enable multichannel binaural rendering" FORCE)
set(HOA_BINAURALIZER OFF CACHE BOOL "Enable HOA binaural rendering" FORCE)

# Fetch but don't build yet - we need to patch first
FetchContent_GetProperties(libiamf)
if(NOT libiamf_POPULATED)
  FetchContent_Populate(libiamf)

  # --- PATCH: Fix libiamf's CMakeLists.txt to remove GCC-only flags and m library on Windows
  if(WIN32)
    file(READ "${libiamf_SOURCE_DIR}/code/CMakeLists.txt" LIBIAMF_CMAKE_CONTENT)
    string(REPLACE "-O3" "" LIBIAMF_CMAKE_CONTENT "${LIBIAMF_CMAKE_CONTENT}")
    string(REPLACE "-Werror=unused-variable" "" LIBIAMF_CMAKE_CONTENT "${LIBIAMF_CMAKE_CONTENT}")
    string(REGEX REPLACE "target_link_libraries\\([^)]*[ \t\n]+m[ \t\n)]+[^)]*\\)" "" LIBIAMF_CMAKE_CONTENT "${LIBIAMF_CMAKE_CONTENT}")
    string(REPLACE " m)" ")" LIBIAMF_CMAKE_CONTENT "${LIBIAMF_CMAKE_CONTENT}")
    string(REPLACE " m " " " LIBIAMF_CMAKE_CONTENT "${LIBIAMF_CMAKE_CONTENT}")
    string(REPLACE "option(BUILD_SHARED_LIBS \"Build shared library\" ON)" "option(BUILD_SHARED_LIBS \"Build shared library\" OFF)" LIBIAMF_CMAKE_CONTENT "${LIBIAMF_CMAKE_CONTENT}")
    file(WRITE "${libiamf_SOURCE_DIR}/code/CMakeLists.txt" "${LIBIAMF_CMAKE_CONTENT}")
    message(STATUS "Patched libiamf CMakeLists.txt to remove GCC-only flags and m library")
  endif()

  # --- macOS vendor handling
  if(APPLE)
    set(VENDOR_LIB_PATH "${CMAKE_SOURCE_DIR}/third_party/libiamf/lib/macos")
    file(REMOVE
            "${libiamf_SOURCE_DIR}/code/dep_codecs/lib/libopus.a"
            "${libiamf_SOURCE_DIR}/code/dep_codecs/lib/libfdk-aac.a"
            "${libiamf_SOURCE_DIR}/code/dep_codecs/lib/libFLAC.a"
    )
    file(COPY
            "${VENDOR_LIB_PATH}/libopus.a"
            "${VENDOR_LIB_PATH}/libFLAC.a"
            "${VENDOR_LIB_PATH}/libfdk-aac.a"
            "${VENDOR_LIB_PATH}/libogg.a"
            DESTINATION "${libiamf_SOURCE_DIR}/code/dep_codecs/lib/"
    )
    link_directories("${VENDOR_LIB_PATH}")
  endif()

  # --- Windows vendor handling
  if(WIN32)
    message(STATUS "Using Windows codec libraries for IAMF")

    set(VENDOR_LIB_PATH_DEBUG "${CMAKE_SOURCE_DIR}/third_party/libiamf/lib/Windows/Debug")
    set(VENDOR_LIB_PATH_RELEASE "${CMAKE_SOURCE_DIR}/third_party/libiamf/lib/Windows/Release")

    # Copy Debug libraries if they exist
    if(EXISTS "${VENDOR_LIB_PATH_DEBUG}/fdk-aac.lib")
      file(COPY
              "${VENDOR_LIB_PATH_DEBUG}/fdk-aac.lib"
              "${VENDOR_LIB_PATH_DEBUG}/FLAC.lib"
              "${VENDOR_LIB_PATH_DEBUG}/opus.lib"
              "${VENDOR_LIB_PATH_DEBUG}/ogg.lib"
              DESTINATION "${libiamf_SOURCE_DIR}/code/dep_codecs/lib/"
      )
    endif()

    # Copy Release libraries if they exist
    if(EXISTS "${VENDOR_LIB_PATH_RELEASE}/fdk-aac.lib")
      file(COPY
              "${VENDOR_LIB_PATH_RELEASE}/fdk-aac.lib"
              "${VENDOR_LIB_PATH_RELEASE}/FLAC.lib"
              "${VENDOR_LIB_PATH_RELEASE}/opus.lib"
              "${VENDOR_LIB_PATH_RELEASE}/ogg.lib"
              DESTINATION "${libiamf_SOURCE_DIR}/code/dep_codecs/lib/"
      )
    endif()
  endif()


  # --- Protobuf / Abseil injection (for Windows builds)
  if(WIN32)
    message(STATUS "Using Protobuf and Abseil targets from FetchContent")
  endif()

  # --- NOW build libiamf (after patching)
  add_subdirectory(${libiamf_SOURCE_DIR}/code ${libiamf_BINARY_DIR})

  # --- PATCH: Remove Linux-only 'm' linkage (no libm on Windows)
  if (WIN32 AND TARGET iamf)
    message(STATUS "Patching libiamf: removing Linux-only 'm' linkage")
    get_target_property(_iamf_libs iamf INTERFACE_LINK_LIBRARIES)
    if (_iamf_libs)
      list(REMOVE_ITEM _iamf_libs m)
      set_target_properties(iamf PROPERTIES INTERFACE_LINK_LIBRARIES "${_iamf_libs}")
    endif()
  endif()

  # --- Windows codec linking (after target exists)
  if(WIN32 AND TARGET iamf)
    target_link_libraries(iamf
            "${VENDOR_LIB_PATH}/fdk-aac.lib"
            "${VENDOR_LIB_PATH}/FLAC.lib"
            "${VENDOR_LIB_PATH}/opus.lib"
            "${VENDOR_LIB_PATH}/ogg.lib"
    )
  endif()
endif()

# --- Extra linking for ogg
if(TARGET iamf)
  target_link_libraries(iamf ogg)
endif()

# --- Include dirs (for iamfdec_utils)
set(LIBIAMF_INCLUDE_DIRS
        ${libiamf_SOURCE_DIR}/code/include
        ${libiamf_SOURCE_DIR}/code/src/common
        ${libiamf_SOURCE_DIR}/code/src/iamf_dec
        ${libiamf_SOURCE_DIR}/code/src/iamf_dec/arch
        ${libiamf_SOURCE_DIR}/code/src/iamf_dec/arch/arm
        ${libiamf_SOURCE_DIR}/code/src/iamf_dec/arch/x86
        ${libiamf_SOURCE_DIR}/code/src/iamf_dec/aac
        ${libiamf_SOURCE_DIR}/code/src/iamf_dec/flac
        ${libiamf_SOURCE_DIR}/code/src/iamf_dec/opus
        ${libiamf_SOURCE_DIR}/code/src/iamf_dec/pcm
        ${libiamf_SOURCE_DIR}/code/dep_external/include/wav
        ${libiamf_SOURCE_DIR}/code/test/tools/iamfdec/include
        CACHE INTERNAL "LIBIAMF include directories"
)

# --- Build iamfdec_utils (no main)
if(EXISTS "${libiamf_SOURCE_DIR}/code/test/tools/iamfdec/src")
  set(IAMFDEC_UTIL_SOURCES
          ${libiamf_SOURCE_DIR}/code/test/tools/iamfdec/src/dmemory.c
          ${libiamf_SOURCE_DIR}/code/test/tools/iamfdec/src/iamf_header.c
          ${libiamf_SOURCE_DIR}/code/test/tools/iamfdec/src/mp4demux.c
          ${libiamf_SOURCE_DIR}/code/test/tools/iamfdec/src/mp4iamfpar.c
          ${libiamf_SOURCE_DIR}/code/test/tools/iamfdec/src/vlogging_iamfmp4_sr.c
  )

  if(IAMFDEC_UTIL_SOURCES)
    add_library(iamfdec_utils STATIC ${IAMFDEC_UTIL_SOURCES})
    target_include_directories(iamfdec_utils PUBLIC
            ${libiamf_SOURCE_DIR}/code/test/tools/iamfdec/include
            $<TARGET_PROPERTY:iamf,INTERFACE_INCLUDE_DIRECTORIES>
    )
    target_include_directories(iamfdec_utils PRIVATE ${LIBIAMF_INCLUDE_DIRS})
    if(TARGET iamf)
      target_link_libraries(iamfdec_utils PUBLIC iamf)
    else()
      message(WARNING "Core 'iamf' target not found for linking iamfdec_utils.")
    endif()
    message(STATUS "Created iamfdec_utils library (without main).")
  endif()
endif()

message(STATUS "LIBIAMF source directory: ${libiamf_SOURCE_DIR}")