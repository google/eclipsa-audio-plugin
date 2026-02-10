# Copyright 2025 Google LLC
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to add writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

#====================================================================
# Include Guard
#====================================================================
if (DEFINED _MACOS_TOOLCHAIN_INCLUDED)
    return()
endif ()
set(_MACOS_TOOLCHAIN_INCLUDED TRUE)

#====================================================================
# Platform Identifier
#====================================================================
set(ECLIPSA_PLATFORM "macos" CACHE STRING "")

#====================================================================
# Deployment Target
#====================================================================
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.15" CACHE STRING "Minimum macOS version")

#====================================================================
# RPATH Configuration
#====================================================================
get_filename_component(_ECLIPSA_ROOT "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)
set(CMAKE_BUILD_RPATH
        "${_ECLIPSA_ROOT}"
        "@loader_path/../Resources"
        "${_ECLIPSA_ROOT}/third_party/gpac/lib"
        "${_ECLIPSA_ROOT}/third_party/iamftools/lib"
        "${_ECLIPSA_ROOT}/third_party/obr/lib"
        CACHE STRING ""
)
#====================================================================
# Linker Settings
#====================================================================
add_link_options("-Wl,-ld_classic")

#====================================================================
# AAX SDK Path
#====================================================================
if (BUILD_AAX)
    set(AAX_SDK_VER "2-7-0" CACHE STRING "AAX SDK Version")
    set(AAX_SDK_SEARCH_HINT "/opt/aax-sdk-${AAX_SDK_VER}" CACHE INTERNAL "")
endif ()

#====================================================================
# Build Settings
#====================================================================
set(IAMF_LIB_NAME "libiamf" CACHE STRING "")
set(ECLIPSA_IAMF_LIB_DIR "${CMAKE_BINARY_DIR}/_deps/libiamf-build" CACHE STRING "")
set(ECLIPSA_STATIC_LIB_SUFFIX ".a" CACHE STRING "")
set(ECLIPSA_PLATFORM_PLUGIN_FORMATS AU CACHE STRING "")
set(ECLIPSA_PLATFORM_LIBS vendored_obr CACHE STRING "")
set(SAF_PERFORMANCE_LIB "SAF_USE_APPLE_ACCELERATE" CACHE STRING "")
set(LIBIAMF_VENDOR_PATH "${CMAKE_SOURCE_DIR}/third_party/libiamf/lib/macos" CACHE PATH "")

#====================================================================
# Bundle Structure
#====================================================================
set(ECLIPSA_BUNDLE_RESOURCES_SUBDIR "Resources" CACHE STRING "")