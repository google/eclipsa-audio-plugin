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

message(STATUS "Fetching LibIAMF")

include(FetchContent)

FetchContent_Declare(
        libiamf
        URL https://github.com/AOMediaCodec/libiamf/archive/48b8b5dc06971e14826d114fd032dab285f4c944.zip
)

set(CODEC_CAP ON CACHE BOOL "Codec capability check" FORCE)
set(MULTICHANNEL_BINAURALIZER OFF CACHE BOOL "Enable multichannel binaural rendering" FORCE)
set(HOA_BINAURALIZER OFF CACHE BOOL "Enable HOA binaural rendering" FORCE)

FetchContent_GetProperties(libiamf)
if (NOT libiamf_POPULATED)
    FetchContent_Populate(libiamf)
    #====================================================================
    # Platform-specific configuration
    #====================================================================
    if (APPLE)
        set(LIBIAMF_CODEC_PATH "${CMAKE_SOURCE_DIR}/third_party/libiamf/lib/macos")
        file(REMOVE
                "${libiamf_SOURCE_DIR}/code/dep_codecs/lib/libopus.a"
                "${libiamf_SOURCE_DIR}/code/dep_codecs/lib/libfdk-aac.a"
                "${libiamf_SOURCE_DIR}/code/dep_codecs/lib/libFLAC.a"
        )
        file(COPY
                "${LIBIAMF_CODEC_PATH}/libopus.a"
                "${LIBIAMF_CODEC_PATH}/libFLAC.a"
                "${LIBIAMF_CODEC_PATH}/libfdk-aac.a"
                "${LIBIAMF_CODEC_PATH}/libogg.a"
                DESTINATION "${libiamf_SOURCE_DIR}/code/dep_codecs/lib/"
        )
        link_directories("${LIBIAMF_CODEC_PATH}")

    elseif (WIN32)
        # Patch CMakeLists.txt to remove GCC-only flags
        file(READ "${libiamf_SOURCE_DIR}/code/CMakeLists.txt" LIBIAMF_CMAKE_CONTENT)
        string(REPLACE "-O3" "" LIBIAMF_CMAKE_CONTENT "${LIBIAMF_CMAKE_CONTENT}")
        string(REPLACE "-Werror=unused-variable" "" LIBIAMF_CMAKE_CONTENT "${LIBIAMF_CMAKE_CONTENT}")
        string(REGEX REPLACE "target_link_libraries\\([^)]*[ \t\n]+m[ \t\n)]+[^)]*\\)" "" LIBIAMF_CMAKE_CONTENT "${LIBIAMF_CMAKE_CONTENT}")
        string(REPLACE " m)" ")" LIBIAMF_CMAKE_CONTENT "${LIBIAMF_CMAKE_CONTENT}")
        string(REPLACE " m " " " LIBIAMF_CMAKE_CONTENT "${LIBIAMF_CMAKE_CONTENT}")
        string(REPLACE "option(BUILD_SHARED_LIBS \"Build shared library\" ON)" "option(BUILD_SHARED_LIBS \"Build shared library\" OFF)" LIBIAMF_CMAKE_CONTENT "${LIBIAMF_CMAKE_CONTENT}")
        file(WRITE "${libiamf_SOURCE_DIR}/code/CMakeLists.txt" "${LIBIAMF_CMAKE_CONTENT}")
        message(STATUS "Patched libiamf CMakeLists.txt for Windows")

        set(LIBIAMF_CODEC_PATH "${CMAKE_SOURCE_DIR}/third_party/libiamf/lib/Windows/Release")
        file(COPY
                "${LIBIAMF_CODEC_PATH}/fdk-aac.lib"
                "${LIBIAMF_CODEC_PATH}/FLAC.lib"
                "${LIBIAMF_CODEC_PATH}/opus.lib"
                "${LIBIAMF_CODEC_PATH}/ogg.lib"
                DESTINATION "${libiamf_SOURCE_DIR}/code/dep_codecs/lib/"
        )
    endif ()

    # Build libiamf
    add_subdirectory(${libiamf_SOURCE_DIR}/code ${libiamf_BINARY_DIR})

    # Windows: remove 'm' linkage and add codec libs
    if (WIN32 AND TARGET iamf)
        get_target_property(_iamf_libs iamf INTERFACE_LINK_LIBRARIES)
        if (_iamf_libs)
            list(REMOVE_ITEM _iamf_libs m)
            set_target_properties(iamf PROPERTIES INTERFACE_LINK_LIBRARIES "${_iamf_libs}")
        endif ()
        target_link_libraries(iamf
                "${LIBIAMF_CODEC_PATH}/fdk-aac.lib"
                "${LIBIAMF_CODEC_PATH}/FLAC.lib"
                "${LIBIAMF_CODEC_PATH}/opus.lib"
                "${LIBIAMF_CODEC_PATH}/ogg.lib"
        )
    endif ()
endif ()

# Extra linking for ogg (Mac/Linux only)
# Windows is already handled explicitly above with full paths
if (TARGET iamf AND NOT WIN32)
    target_link_libraries(iamf ogg)
endif ()

# Include dirs
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

# Build iamfdec_utils
if (EXISTS "${libiamf_SOURCE_DIR}/code/test/tools/iamfdec/src")
    set(IAMFDEC_UTIL_SOURCES
            ${libiamf_SOURCE_DIR}/code/test/tools/iamfdec/src/dmemory.c
            ${libiamf_SOURCE_DIR}/code/test/tools/iamfdec/src/iamf_header.c
            ${libiamf_SOURCE_DIR}/code/test/tools/iamfdec/src/mp4demux.c
            ${libiamf_SOURCE_DIR}/code/test/tools/iamfdec/src/mp4iamfpar.c
            ${libiamf_SOURCE_DIR}/code/test/tools/iamfdec/src/vlogging_iamfmp4_sr.c
    )

    add_library(iamfdec_utils STATIC ${IAMFDEC_UTIL_SOURCES})
    target_include_directories(iamfdec_utils PUBLIC
            ${libiamf_SOURCE_DIR}/code/test/tools/iamfdec/include
            $<TARGET_PROPERTY:iamf,INTERFACE_INCLUDE_DIRECTORIES>
    )
    target_include_directories(iamfdec_utils PRIVATE ${LIBIAMF_INCLUDE_DIRS})
    target_link_libraries(iamfdec_utils PUBLIC iamf)
    message(STATUS "Created iamfdec_utils library (without main).")
endif ()

message(STATUS "LIBIAMF source directory: ${libiamf_SOURCE_DIR}")