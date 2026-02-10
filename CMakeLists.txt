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

cmake_minimum_required(VERSION 3.30.0)

#====================================================================
#Cmake Policies
#====================================================================
cmake_policy(SET CMP0167 NEW)
cmake_policy(SET CMP0077 NEW)
cmake_policy(SET CMP0169 OLD)

#====================================================================
# Disable vcpkg from finding Boost and Protobuf (provided by libiamf)
#====================================================================
set(CMAKE_DISABLE_FIND_PACKAGE_Boost ON CACHE BOOL "" FORCE)
set(CMAKE_DISABLE_FIND_PACKAGE_Protobuf ON CACHE BOOL "" FORCE)

#====================================================================
# Clear any cached Protobuf references
#====================================================================
unset(Protobuf_INCLUDE_DIRS CACHE)
unset(Protobuf_LIBRARIES CACHE)
unset(Protobuf_DIR CACHE)
unset(Protobuf_FOUND CACHE)
unset(Protobuf_PROTOC_EXECUTABLE CACHE)

#====================================================================
# Version configuration
# The version can be set in multiple ways (in order of precedence):
# 1. Pass via command line: cmake -DECLIPSA_VERSION=1.2.3 ...
# 2. Set via environment variable: export ECLIPSA_VERSION=1.2.3
# 3. Extract from git tag: git describe --tags (requires manual implementation)
# 4. Default to "0.0.1" if none of the above are set
#
# For automated builds/installers, it's recommended to:
# - Pass the version explicitly via command line flag (-DECLIPSA_VERSION=x.y.z)
# - Or extract from git tag in your build script and pass it to CMake
#
# The version appears as a watermark in the bottom-left corner of both plugins.
#====================================================================
if (NOT DEFINED ECLIPSA_VERSION)
    set(ECLIPSA_VERSION "0.0.1" CACHE STRING "Version for Eclipsa project")
endif ()

project(Eclipsa LANGUAGES C CXX VERSION ${ECLIPSA_VERSION})

#====================================================================
# Compiler Settings
#====================================================================
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_EXTENSIONS OFF)

#====================================================================
# Plugin Formats
#====================================================================
set(ECLIPSA_PLUGIN_FORMATS Standalone)

if (BUILD_VST3)
    list(APPEND ECLIPSA_PLUGIN_FORMATS VST3)
endif ()
if (BUILD_AAX)
    list(APPEND ECLIPSA_PLUGIN_FORMATS AAX)
endif ()

list(APPEND ECLIPSA_PLUGIN_FORMATS ${ECLIPSA_PLATFORM_PLUGIN_FORMATS})

#====================================================================
# Build Configuration
#====================================================================
set(BUILD_LIB_DIR "$<IF:$<CONFIG:Debug>,Debug,Release>")
add_compile_definitions(ECLIPSA_VERSION="${ECLIPSA_VERSION}")

#====================================================================
# Plugin Naming
#====================================================================
# Make company name configurable via command line with a default value
if (NOT DEFINED ECLIPSA_COMPANY_NAME)
    set(ECLIPSA_COMPANY_NAME "Eclipsa Project" CACHE STRING "Company name for Eclipsa project")
endif ()
# Make manufacturer code configurable via command line with a default value
if (NOT DEFINED ECLIPSA_MANUFACTURER_CODE)
    set(ECLIPSA_MANUFACTURER_CODE "Eclp" CACHE STRING "Manufacturer code for Eclipsa project")
endif ()
# Make bundle IDs configurable via command line with default values
if (NOT DEFINED ECLIPSA_RENDERER_BUNDLE_ID)
    set(ECLIPSA_RENDERER_BUNDLE_ID "com.eclipsaproject.renderer" CACHE STRING "Bundle ID for the renderer plugin")
endif ()
if (NOT DEFINED ECLIPSA_PANNER_BUNDLE_ID)
    set(ECLIPSA_PANNER_BUNDLE_ID "com.eclipsaproject.panner" CACHE STRING "Bundle ID for the panner plugin")
endif ()

#====================================================================
# Build Options
#====================================================================
option(ECLIPSA_LOGIC_PRO_BUILD "Build AU plugin optimized for Logic Pro (7.1.4 layout)" OFF)
option(INTERNAL_TEST OFF)
option(CI_TEST OFF)
option(JUCE_COPY_PLUGIN_AFTER_BUILD "Automatically copy built plugins to standard locations after build" ON)

#====================================================================
# Testing
#====================================================================
if (CI_TEST OR INTERNAL_TEST)
    message(STATUS "Unit tests enabled")
    include(FetchContent)
    FetchContent_Declare(
            googletest
            GIT_REPOSITORY https://github.com/google/googletest.git
            GIT_TAG v1.14.0
    )
    FetchContent_MakeAvailable(GoogleTest)
    enable_testing()
    include(GoogleTest)
    include("cmake/eclipsa_test.cmake")
    include("cmake/eclipsa_build_tests.cmake")

    # Check for an FFmpeg install for tests that do additional media verification
    include("cmake/find_ffmpeg.cmake")
    if (FFMPEG_TOOLS_FOUND)
        message(STATUS "FFmpeg found. Enabling tests that require FFmpeg.")
        add_compile_definitions(ECLIPSA_FFMPEG_AVAILABLE=1)
    else ()
        message(STATUS "FFmpeg not found. Tests requiring FFmpeg will be disabled.")
    endif ()

    # FFmpeg is required for CI tests
    if (CI_TEST AND NOT FFMPEG_TOOLS_FOUND)
        message(FATAL_ERROR "FFmpeg tools not found. Required for CI tests.")
    endif ()
endif ()

#====================================================================
# Cmake Functions
#====================================================================
include("${CMAKE_SOURCE_DIR}/cmake/copy_resources.cmake")
include("${CMAKE_SOURCE_DIR}/cmake/eclipsa_plugin_defaults.cmake")

#====================================================================
# Dependencies
#====================================================================
add_subdirectory(third_party)
if(WIN32)
    find_package(ZLIB REQUIRED)
    link_libraries(ZLIB::ZLIB)
endif()
#====================================================================
# AAX Setup
#====================================================================
if (BUILD_AAX)
    find_path(AAX_SDK_ROOT
            NAMES "Interfaces/AAX_Exports.cpp"
            HINTS "${AAX_SDK_SEARCH_HINT}"
            DOC "Path to AAX SDK"
    )

    if (AAX_SDK_ROOT)
        message(STATUS "AAX SDK found: ${AAX_SDK_ROOT}")
        juce_set_aax_sdk_path("${AAX_SDK_ROOT}")
    else ()
        message(FATAL_ERROR "AAX SDK not found. Please set AAX_SDK_ROOT.")
    endif ()
endif ()

#====================================================================
# Add Sub Directories
#====================================================================
add_subdirectory(common)
message(STATUS "ECLIPSA_PLUGIN_FORMATS = ${ECLIPSA_PLUGIN_FORMATS}")
add_subdirectory(rendererplugin)
add_subdirectory(audioelementplugin)

#====================================================================
# Dependency List
#====================================================================
if (NOT DEFINED ECLIPSA_PLATFORM_LIBS)
    message(WARNING "ECLIPSA_PLATFORM_LIBS not set—are you using a platform toolchain?")
    set(ECLIPSA_PLATFORM_LIBS "")
endif ()

add_library(eclipsa_dependencies INTERFACE)

target_link_libraries(eclipsa_dependencies INTERFACE
        # Vendored shared libs
        vendored_gpac
        vendored_iamf_tools
        libzmq

        # Static libs from third_party
        saf
        Libear
        LUFSMeter
        spatialaudiolib
        ${IAMF_LIB_NAME}

        # Platform-specific (from toolchain)
        ${ECLIPSA_PLATFORM_LIBS}
)

#====================================================================
# Testing #2
#====================================================================
if (CI_TEST OR INTERNAL_TEST)
    eclipsa_build_tests()
endif ()