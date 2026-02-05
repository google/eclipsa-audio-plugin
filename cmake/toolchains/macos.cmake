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
# Deployment Target
#====================================================================
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.15" CACHE STRING "Minimum macOS version")

#====================================================================
# RPATH Configuration
#====================================================================
set(CMAKE_BUILD_RPATH "@loader_path/../Resources;${CMAKE_SOURCE_DIR}" CACHE STRING "")

#====================================================================
# Linker Settings
#====================================================================
# Xcode 15+ classic linker (avoids duplicate symbol warnings)
add_link_options("-Wl,-ld_classic")

#====================================================================
# AAX SDK Path
#====================================================================
if (BUILD_AAX)
    set(AAX_SDK_VER "2-7-0" CACHE STRING "AAX SDK Version")
    set(AAX_SDK_SEARCH_HINT "/opt/aax-sdk-${AAX_SDK_VER}" CACHE INTERNAL "")
endif ()

#====================================================================
# Library Names
#====================================================================
set(IAMF_LIB_NAME "libiamf" CACHE STRING "")

set(ECLIPSA_PLATFORM_LIBS
        vendored_obr
        CACHE STRING "Platform-specific libraries"
)

set(ECLIPSA_PLATFORM_PLUGIN_FORMATS AU CACHE STRING "Platform-specific plugin formats")

set(ECLIPSA_IAMF_LIB_DIR "${CMAKE_BINARY_DIR}/_deps/libiamf-build" CACHE STRING "")

set(SAF_PERFORMANCE_LIB "SAF_USE_APPLE_ACCELERATE" CACHE STRING "")

set(ECLIPSA_STATIC_LIB_SUFFIX ".a" CACHE STRING "")

set(ECLIPSA_PLATFORM "macos" CACHE STRING "")
