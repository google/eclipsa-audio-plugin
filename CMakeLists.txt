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

cmake_minimum_required(VERSION 3.29.0)

if(NOT DEFINED ECLIPSA_VERSION)
    set(ECLIPSA_VERSION "0.0.1" CACHE STRING "Version for Eclipsa project")
endif()
# Set the version information here so it can be used for determining
# the configuration version
add_compile_definitions(ECLIPSA_VERSION="${ECLIPSA_VERSION}")

project(Eclipsa LANGUAGES C CXX VERSION ${ECLIPSA_VERSION})

# Make company name configurable via command line with a default value
if(NOT DEFINED ECLIPSA_COMPANY_NAME)
    set(ECLIPSA_COMPANY_NAME "Eclipsa Project" CACHE STRING "Company name for Eclipsa project")
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
set(PLUGIN_FORMATS "AU")
option(INTERNAL_TEST OFF)
option(CI_TEST OFF)
if(APPLE)
  # Add vendored libraries path (for CI/GitHub Actions)
  set(VENDOR_LIB_PATH "${CMAKE_SOURCE_DIR}/third_party/libiamf/third_party/lib/macos")
  link_directories(${VENDOR_LIB_PATH})
endif()

if(CI_TEST OR INTERNAL_TEST)
    message(STATUS "Unit tests enabled")
    include(FetchContent)    
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY
        https://github.com/google/googletest.git
        GIT_TAG v1.14.0)
    FetchContent_MakeAvailable(GoogleTest)
    enable_testing()
    include(GoogleTest)
    include("cmake/eclipsa_test.cmake")
    include("cmake/eclipsa_build_tests.cmake")
endif()

add_subdirectory(third_party)
include("${CMAKE_SOURCE_DIR}/cmake/copy_resources.cmake")

if(BUILD_AAX)
    set(AAX_SDK_VER "2-7-0" CACHE STRING "Default AAX SDK Version")
    list(APPEND PLUGIN_FORMATS "AAX")
    find_path(AAX_SDK_PATH NAMES "aax-sdk-${AAX_SDK_VER}" HINTS "/opt" "/usr/local")
    if(${AAX_SDK_PATH} STREQUAL AAX_SDK_PATH-NOTFOUND)
        message(FATAL_ERROR "Could not find AAX_SDK at hinted locations")
    else()
        message(STATUS "AAX SDK found at " ${AAX_SDK_PATH})
        juce_set_aax_sdk_path("${AAX_SDK_PATH}/aax-sdk-${AAX_SDK_VER}")
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