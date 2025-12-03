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

if(WIN32)
    # Disable vcpkg automatic DLL copying (applocal) - it fails without dumpbin in PATH
    set(X_VCPKG_APPLOCAL_DEPS_INSTALL OFF CACHE BOOL "")

    # Increase stack size to 16MB to prevent stack overflows
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /STACK:16777216")
endif()

# Disable vcpkg from finding Boost and Protobuf (provided by libiamf)
set(CMAKE_DISABLE_FIND_PACKAGE_Boost ON CACHE BOOL "" FORCE)
set(CMAKE_DISABLE_FIND_PACKAGE_Protobuf ON CACHE BOOL "" FORCE)

# Clear any cached Protobuf references
unset(Protobuf_INCLUDE_DIRS CACHE)
unset(Protobuf_LIBRARIES CACHE)
unset(Protobuf_DIR CACHE)
unset(Protobuf_FOUND CACHE)
unset(Protobuf_PROTOC_EXECUTABLE CACHE)

cmake_policy(SET CMP0167 NEW)
cmake_policy(SET CMP0077 NEW)
cmake_policy(SET CMP0169 OLD)
set(CMAKE_CXX_EXTENSIONS OFF)

# Configure release or debug builds
set(BUILD_LIB_DIR "$<IF:$<CONFIG:Debug>,Debug,Release>")

# Windows vcpkg setup
if(WIN32 AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    # Try environment variable first (respects user's existing setup)
    if(DEFINED ENV{VCPKG_ROOT})
        set(VCPKG_ROOT "$ENV{VCPKG_ROOT}" CACHE PATH "Path to vcpkg root")
    endif()

    # Configure vcpkg if VCPKG_ROOT is set
    if(DEFINED VCPKG_ROOT)
        set(CMAKE_TOOLCHAIN_FILE "${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE FILEPATH "vcpkg toolchain file")

        # Validate toolchain file exists
        if(NOT EXISTS "${CMAKE_TOOLCHAIN_FILE}")
            message(FATAL_ERROR "vcpkg toolchain file not found at: ${CMAKE_TOOLCHAIN_FILE}")
        endif()

        # Set default triplet if not specified
        if(NOT DEFINED VCPKG_TARGET_TRIPLET)
            set(VCPKG_TARGET_TRIPLET "x64-windows" CACHE STRING "vcpkg target triplet")
        endif()

        message(WARNING "vcpkg toolchain: ${CMAKE_TOOLCHAIN_FILE}")
        message(WARNING "vcpkg triplet: ${VCPKG_TARGET_TRIPLET}")
    else()
        message(WARNING "vcpkg not configured. Please either:")
        message(WARNING "  1. Set VCPKG_ROOT environment variable, or")
        message(WARNING "  2. Pass -DVCPKG_ROOT=<path> to CMake")
        message(WARNING "Build will continue without vcpkg - dependencies must be installed manually")
    endif()
endif()

if(NOT DEFINED ECLIPSA_VERSION)
    set(ECLIPSA_VERSION "0.0.1" CACHE STRING "Version for Eclipsa project")
endif()

project(Eclipsa LANGUAGES C CXX VERSION ${ECLIPSA_VERSION})

add_compile_definitions(ECLIPSA_VERSION="${ECLIPSA_VERSION}")

# Make company name configurable via command line with a default value
if(NOT DEFINED ECLIPSA_COMPANY_NAME)
    set(ECLIPSA_COMPANY_NAME "Eclipsa Project" CACHE STRING "Company name for Eclipsa project")
endif()

if(WIN32 AND NOT CMAKE_RC_COMPILER)
    find_program(CMAKE_RC_COMPILER rc.exe
            HINTS "C:/Program Files (x86)/Windows Kits/10/bin/*/x64")
endif()

# Make manufacturer code configurable via command line with a default value
if(NOT DEFINED ECLIPSA_MANUFACTURER_CODE)
    set(ECLIPSA_MANUFACTURER_CODE "Eclp" CACHE STRING "Manufacturer code for Eclipsa project")
endif()

# Make bundle IDs configurable via command line with default values
if(NOT DEFINED ECLIPSA_RENDERER_BUNDLE_ID)
    set(ECLIPSA_RENDERER_BUNDLE_ID "com.eclipsaproject.renderer" CACHE STRING "Bundle ID for the renderer plugin")
endif()

if(NOT DEFINED ECLIPSA_PANNER_BUNDLE_ID)
    set(ECLIPSA_PANNER_BUNDLE_ID "com.eclipsaproject.panner" CACHE STRING "Bundle ID for the panner plugin")
endif()

# Build configuration for different DAW targets
option(ECLIPSA_LOGIC_PRO_BUILD "Build AU plugin optimized for Logic Pro (7.1.4 layout)" OFF)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_BUILD_RPATH "@loader_path/../Resources;${CMAKE_SOURCE_DIR}")

# Set default plugin formats per platform
if(APPLE)
    set(PLUGIN_FORMATS "AU")
elseif(WIN32)
    set(PLUGIN_FORMATS "Standalone")
else()
    set(PLUGIN_FORMATS "")
endif()

option(INTERNAL_TEST OFF)
option(CI_TEST OFF)

if(APPLE)
    # Add vendored libraries path (for CI/GitHub Actions)
    set(VENDOR_LIB_PATH "${CMAKE_SOURCE_DIR}/third_party/libiamf/lib/macos")
    link_directories(${VENDOR_LIB_PATH})
endif()

if(CI_TEST OR INTERNAL_TEST)
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
endif()

add_subdirectory(third_party)

# ZLIB configuration for Windows builds
if(WIN32)
    find_package(ZLIB REQUIRED)
    message(STATUS "ZLIB found at: ${ZLIB_LIBRARIES}")
    # Fix link targets that request 'z'
    link_libraries(ZLIB::ZLIB)
endif()

include("${CMAKE_SOURCE_DIR}/cmake/copy_resources.cmake")

if(BUILD_AAX)
    if(WIN32)
        set(AAX_SDK_VER "2-8-1" CACHE STRING "AAX SDK Version")
        list(APPEND PLUGIN_FORMATS "AAX")

        # Allow team members to specify their own AAX SDK path
        if(NOT DEFINED AAX_SDK_ROOT)
            set(AAX_SDK_ROOT "C:/Code/Repos/aax-sdk-${AAX_SDK_VER}" CACHE PATH "Path to AAX SDK")
        endif()
    else()
        set(AAX_SDK_VER "2-7-0" CACHE STRING "Default AAX SDK Version")
        list(APPEND PLUGIN_FORMATS "AAX")
        find_path(AAX_SDK_ROOT NAMES "aax-sdk-${AAX_SDK_VER}" HINTS "/opt" "/usr/local")
        set(AAX_SDK_ROOT "${AAX_SDK_ROOT}/aax-sdk-${AAX_SDK_VER}")
    endif()

    if(EXISTS "${AAX_SDK_ROOT}")
        message(STATUS "Using AAX SDK at ${AAX_SDK_ROOT}")
        juce_set_aax_sdk_path("${AAX_SDK_ROOT}")
    else()
        message(FATAL_ERROR "AAX SDK not found at ${AAX_SDK_ROOT}. Set AAX_SDK_ROOT to the correct path.")
    endif()
endif()

if(BUILD_VST3)
    list(APPEND PLUGIN_FORMATS "VST3")
endif()

add_subdirectory(common)
add_subdirectory(rendererplugin)
add_subdirectory(audioelementplugin)

if (CI_TEST OR INTERNAL_TEST)
    eclipsa_build_tests()
endif()