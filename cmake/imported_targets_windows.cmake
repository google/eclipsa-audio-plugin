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
# Imported Targets - Windows
#====================================================================

# GPAC
if (NOT TARGET vendored_gpac)
    add_library(vendored_gpac SHARED IMPORTED GLOBAL)
    set_target_properties(vendored_gpac PROPERTIES
            IMPORTED_LOCATION_RELEASE "${CMAKE_SOURCE_DIR}/third_party/gpac/lib/Windows/Release/libgpac.dll"
            IMPORTED_LOCATION_DEBUG "${CMAKE_SOURCE_DIR}/third_party/gpac/lib/Windows/Debug/libgpac.dll"
            IMPORTED_IMPLIB_RELEASE "${CMAKE_SOURCE_DIR}/third_party/gpac/lib/Windows/Release/libgpac.lib"
            IMPORTED_IMPLIB_DEBUG "${CMAKE_SOURCE_DIR}/third_party/gpac/lib/Windows/Debug/libgpac.lib"
    )
endif ()

# IAMF Tools
if (NOT TARGET vendored_iamf_tools)
    add_library(vendored_iamf_tools SHARED IMPORTED GLOBAL)
    set_target_properties(vendored_iamf_tools PROPERTIES
            IMPORTED_LOCATION_RELEASE "${CMAKE_SOURCE_DIR}/third_party/iamftools/lib/Windows/Release/iamf_tools.dll"
            IMPORTED_LOCATION_DEBUG "${CMAKE_SOURCE_DIR}/third_party/iamftools/lib/Windows/Debug/iamf_tools.dll"
            IMPORTED_IMPLIB_RELEASE "${CMAKE_SOURCE_DIR}/third_party/iamftools/lib/Windows/Release/iamf_tools.lib"
            IMPORTED_IMPLIB_DEBUG "${CMAKE_SOURCE_DIR}/third_party/iamftools/lib/Windows/Debug/iamf_tools.lib"
    )
endif ()

# GPAC Crypto
if (NOT TARGET vendored_gpac_crypto)
    add_library(vendored_gpac_crypto SHARED IMPORTED GLOBAL)
    set_target_properties(vendored_gpac_crypto PROPERTIES
            IMPORTED_LOCATION_RELEASE "${CMAKE_SOURCE_DIR}/third_party/gpac/lib/Windows/Release/libcryptoMD.dll"
            IMPORTED_LOCATION_DEBUG "${CMAKE_SOURCE_DIR}/third_party/gpac/lib/Windows/Debug/libcryptoMD.dll"
    )
endif ()

# GPAC SSL
if (NOT TARGET vendored_gpac_ssl)
    add_library(vendored_gpac_ssl SHARED IMPORTED GLOBAL)
    set_target_properties(vendored_gpac_ssl PROPERTIES
            IMPORTED_LOCATION_RELEASE "${CMAKE_SOURCE_DIR}/third_party/gpac/lib/Windows/Release/libsslMD.dll"
            IMPORTED_LOCATION_DEBUG "${CMAKE_SOURCE_DIR}/third_party/gpac/lib/Windows/Debug/libsslMD.dll"
    )
endif ()