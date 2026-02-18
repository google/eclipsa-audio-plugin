# Copyright 2025 Google LLC
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.

#====================================================================
# Prebuilt Libraries
#====================================================================

# GPAC
add_library(gpac_impl SHARED IMPORTED GLOBAL)
set_target_properties(gpac_impl PROPERTIES
        IMPORTED_LOCATION_RELEASE "${CMAKE_SOURCE_DIR}/third_party/gpac/lib/Windows/Release/libgpac.dll"
        IMPORTED_LOCATION_DEBUG "${CMAKE_SOURCE_DIR}/third_party/gpac/lib/Windows/Debug/libgpac.dll"
        IMPORTED_IMPLIB_RELEASE "${CMAKE_SOURCE_DIR}/third_party/gpac/lib/Windows/Release/libgpac.lib"
        IMPORTED_IMPLIB_DEBUG "${CMAKE_SOURCE_DIR}/third_party/gpac/lib/Windows/Debug/libgpac.lib"
)
add_library(vendored_gpac ALIAS gpac_impl)

# IAMF Tools
add_library(iamftools_impl SHARED IMPORTED GLOBAL)
set_target_properties(iamftools_impl PROPERTIES
        IMPORTED_LOCATION_RELEASE "${CMAKE_SOURCE_DIR}/third_party/iamftools/lib/Windows/Release/iamf_tools.dll"
        IMPORTED_LOCATION_DEBUG "${CMAKE_SOURCE_DIR}/third_party/iamftools/lib/Windows/Debug/iamf_tools.dll"
        IMPORTED_IMPLIB_RELEASE "${CMAKE_SOURCE_DIR}/third_party/iamftools/lib/Windows/Release/iamf_tools.if.lib"
        IMPORTED_IMPLIB_DEBUG "${CMAKE_SOURCE_DIR}/third_party/iamftools/lib/Windows/Debug/iamf_tools.if.lib"
)
add_library(vendored_iamf_tools ALIAS iamftools_impl)

# GPAC Crypto
add_library(vendored_gpac_crypto SHARED IMPORTED GLOBAL)
set_target_properties(vendored_gpac_crypto PROPERTIES
        IMPORTED_LOCATION_RELEASE "${CMAKE_SOURCE_DIR}/third_party/gpac/lib/Windows/Release/libcryptoMD.dll"
        IMPORTED_LOCATION_DEBUG "${CMAKE_SOURCE_DIR}/third_party/gpac/lib/Windows/Debug/libcryptoMD.dll"
)

# GPAC SSL
add_library(vendored_gpac_ssl SHARED IMPORTED GLOBAL)
set_target_properties(vendored_gpac_ssl PROPERTIES
        IMPORTED_LOCATION_RELEASE "${CMAKE_SOURCE_DIR}/third_party/gpac/lib/Windows/Release/libsslMD.dll"
        IMPORTED_LOCATION_DEBUG "${CMAKE_SOURCE_DIR}/third_party/gpac/lib/Windows/Debug/libsslMD.dll"
)

# OBR (static)
add_library(obr_impl INTERFACE)
target_link_libraries(obr_impl INTERFACE
        $<$<CONFIG:Debug>:${CMAKE_SOURCE_DIR}/third_party/obr/lib/Windows/Debug/obr.lib>
        $<$<CONFIG:Release>:${CMAKE_SOURCE_DIR}/third_party/obr/lib/Windows/Release/obr.lib>
        $<$<CONFIG:Debug>:${CMAKE_SOURCE_DIR}/third_party/obr/lib/Windows/Debug/pffft.lib>
        $<$<CONFIG:Release>:${CMAKE_SOURCE_DIR}/third_party/obr/lib/Windows/Release/pffft.lib>
)

# Libear
add_library(libear_impl STATIC IMPORTED GLOBAL)
set_target_properties(libear_impl PROPERTIES
        IMPORTED_LOCATION_RELEASE "${CMAKE_SOURCE_DIR}/third_party/libear/lib/Windows/Release/libear.lib"
        IMPORTED_LOCATION_DEBUG "${CMAKE_SOURCE_DIR}/third_party/libear/lib/Windows/Debug/libear.lib"
)

#====================================================================
# Vendored Libraries
#====================================================================
set(ECLIPSA_VENDORED_LIBS
        vendored_gpac
        vendored_iamf_tools
        vendored_gpac_crypto
        vendored_gpac_ssl
        libzmq
        CACHE STRING ""
)