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

function(copy_resources target plugin_path)
    if(APPLE)
        # macOS: Copy dylibs to Resources
        set(LIB_OBR_PATH "${CMAKE_SOURCE_DIR}/third_party/obr/lib/obr.dylib")
        set(LIB_IAMF_TOOLS_PATH "${CMAKE_SOURCE_DIR}/third_party/iamftools/lib/libiamf_tools.dylib")
        set(LIB_ZMQ_5_2_6_PATH "${CMAKE_BINARY_DIR}/_deps/zeromq-build/lib/libzmq.5.2.6.dylib")
        set(LIB_GPAC_PATH "${CMAKE_SOURCE_DIR}/third_party/gpac/lib/libgpac.dylib")

        # Set Resources directory and external subdirectories
        set(RESOURCES_DIR "${plugin_path}/Contents/Resources")
        set(EXTERNAL_IAMF_DIR "${RESOURCES_DIR}/third_party/iamftools/lib")
        set(EXTERNAL_OBR_DIR "${RESOURCES_DIR}/third_party/obr/lib")
        set(EXTERNAL_GPAC_DIR "${RESOURCES_DIR}/third_party/gpac/lib")

        # Copy libraries to the appropriate directories
        add_custom_command(TARGET ${target} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E make_directory ${EXTERNAL_IAMF_DIR}
                COMMAND ${CMAKE_COMMAND} -E make_directory ${EXTERNAL_OBR_DIR}
                COMMAND ${CMAKE_COMMAND} -E make_directory ${EXTERNAL_GPAC_DIR}
                COMMAND ${CMAKE_COMMAND} -E copy ${LIB_OBR_PATH} ${EXTERNAL_OBR_DIR}/obr.dylib
                COMMAND ${CMAKE_COMMAND} -E copy ${LIB_IAMF_TOOLS_PATH} ${EXTERNAL_IAMF_DIR}/libiamf_tools.dylib
                COMMAND ${CMAKE_COMMAND} -E copy ${LIB_GPAC_PATH} ${EXTERNAL_GPAC_DIR}/libgpac.dylib
                COMMAND ${CMAKE_COMMAND} -E copy ${LIB_ZMQ_5_2_6_PATH} ${RESOURCES_DIR}/libzmq.5.2.6.dylib

                # Create symbolic links for libzmq
                COMMAND ${CMAKE_COMMAND} -E create_symlink libzmq.5.2.6.dylib ${RESOURCES_DIR}/libzmq.5.dylib
                COMMAND ${CMAKE_COMMAND} -E create_symlink libzmq.5.dylib ${RESOURCES_DIR}/libzmq.dylib
        )

    elseif(WIN32)
        # Set DLL names using generator expressions for multi-config generators
        set(GPAC_DLL        "libgpac.dll")
        set(CRYPTOMD_DLL    "libcryptoMD.dll")
        set(LIBSSLMD_DLL    "libsslMD.dll")
        set(IAMF_TOOLS_DLL  "iamf_tools.dll")
        set(OPENSVC_DECODER "OpenSVCDecoder.dll")
        set(ZMQ_DLL         "libzmq-v143-mt$<$<CONFIG:Debug>:-gd>-4_3_6.dll")

        # Export DLL list to parent scope for DELAYLOAD configuration
        set(DELAYLOAD_DLLS
                ${GPAC_DLL}
                ${CRYPTOMD_DLL}
                ${LIBSSLMD_DLL}
                ${IAMF_TOOLS_DLL}
                ${OPENSVC_DECODER}
                ${ZMQ_DLL}
                PARENT_SCOPE
        )

        # Set the paths to the DLLs
        set(GPAC_DLL_PATH   "${CMAKE_SOURCE_DIR}/third_party/gpac/lib/Windows/$<CONFIG>/${GPAC_DLL}")
        set(CRYPTOMD_DLL_PATH    "${CMAKE_SOURCE_DIR}/third_party/gpac/lib/Windows/$<CONFIG>/${CRYPTOMD_DLL}")
        set(LIBSSLMD_DLL_PATH    "${CMAKE_SOURCE_DIR}/third_party/gpac/lib/Windows/$<CONFIG>/${LIBSSLMD_DLL}")
        set(IAMF_TOOLS_DLL_PATH  "${CMAKE_SOURCE_DIR}/third_party/iamftools/lib/Windows/$<CONFIG>/${IAMF_TOOLS_DLL}")
        set(OPENSVC_DECODER_PATH "${CMAKE_SOURCE_DIR}/third_party/OpenSVC/lib/Windows/$<CONFIG>/${OPENSVC_DECODER}")
        set(ZMQ_DLL_PATH         "${CMAKE_BINARY_DIR}/_deps/zeromq-build/bin/$<CONFIG>/${ZMQ_DLL}")


        if("${target}" MATCHES ".*_VST3$")
            set(PLUGIN_BINARY_DIR "${plugin_path}/Contents/x86_64-win")
        elseif("${target}" MATCHES ".*_AAX$")
            set(PLUGIN_BINARY_DIR "${plugin_path}/Contents/x64")
        elseif("${target}" MATCHES ".*_Standalone$")
            set(PLUGIN_BINARY_DIR "${plugin_path}")
        else()
            message(WARNING "Unknown plugin format for target: ${target}. Skipping DLL copy.")
            return()
        endif()

        # Single POST_BUILD command per target
        add_custom_command(TARGET ${target} PRE_BUILD
                COMMAND ${CMAKE_COMMAND} -E make_directory "${PLUGIN_BINARY_DIR}"
                COMMAND ${CMAKE_COMMAND} -E copy_if_different ${GPAC_DLL_PATH} "${PLUGIN_BINARY_DIR}/"
                COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CRYPTOMD_DLL_PATH} "${PLUGIN_BINARY_DIR}/"
                COMMAND ${CMAKE_COMMAND} -E copy_if_different ${LIBSSLMD_DLL_PATH} "${PLUGIN_BINARY_DIR}/"
                COMMAND ${CMAKE_COMMAND} -E copy_if_different ${IAMF_TOOLS_DLL_PATH} "${PLUGIN_BINARY_DIR}/"
                COMMAND ${CMAKE_COMMAND} -E copy_if_different ${ZMQ_DLL_PATH} "${PLUGIN_BINARY_DIR}/"
                COMMENT "Copied DLLs to ${PLUGIN_BINARY_DIR}"
        )

        # Copy DLLs to Debug folder in build directory for use by the JUCE VST3 tool
        # The tool runs from this directory and runs the plugins, so the DLLs need to be here as well
        set(VST3_SIGNING_DIR "${CMAKE_CURRENT_BINARY_DIR}/${BUILD_LIB_DIR}")
        add_custom_command(TARGET ${target} PRE_BUILD
                COMMAND ${CMAKE_COMMAND} -E make_directory "${VST3_SIGNING_DIR}"
                COMMAND ${CMAKE_COMMAND} -E copy_if_different ${GPAC_DLL_PATH} "${VST3_SIGNING_DIR}/"
                COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CRYPTOMD_DLL_PATH} "${VST3_SIGNING_DIR}/"
                COMMAND ${CMAKE_COMMAND} -E copy_if_different ${LIBSSLMD_DLL_PATH} "${VST3_SIGNING_DIR}/"
                COMMAND ${CMAKE_COMMAND} -E copy_if_different ${IAMF_TOOLS_DLL_PATH} "${VST3_SIGNING_DIR}/"
                COMMAND ${CMAKE_COMMAND} -E copy_if_different ${ZMQ_DLL_PATH} "${VST3_SIGNING_DIR}/"
                COMMENT "Copied DLLs to ${VST3_SIGNING_DIR}"
        )
    endif()
endfunction()