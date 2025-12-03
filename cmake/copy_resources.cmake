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

        # Set Resources directory and external subdirectories
        set(RESOURCES_DIR "${plugin_path}/Contents/Resources")
        set(EXTERNAL_IAMF_DIR "${RESOURCES_DIR}/third_party/iamftools/lib")
        set(EXTERNAL_OBR_DIR "${RESOURCES_DIR}/third_party/obr/lib")

        # Copy libraries to the appropriate directories
        add_custom_command(TARGET ${target} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E make_directory ${EXTERNAL_IAMF_DIR}
                COMMAND ${CMAKE_COMMAND} -E make_directory ${EXTERNAL_OBR_DIR}
                COMMAND ${CMAKE_COMMAND} -E copy ${LIB_OBR_PATH} ${EXTERNAL_OBR_DIR}/obr.dylib
                COMMAND ${CMAKE_COMMAND} -E copy ${LIB_IAMF_TOOLS_PATH} ${EXTERNAL_IAMF_DIR}/libiamf_tools.dylib
                COMMAND ${CMAKE_COMMAND} -E copy ${LIB_ZMQ_5_2_6_PATH} ${RESOURCES_DIR}/libzmq.5.2.6.dylib

                # Create symbolic links for libzmq
                COMMAND ${CMAKE_COMMAND} -E create_symlink libzmq.5.2.6.dylib ${RESOURCES_DIR}/libzmq.5.dylib
                COMMAND ${CMAKE_COMMAND} -E create_symlink libzmq.5.dylib ${RESOURCES_DIR}/libzmq.dylib
        )

    elseif(WIN32)
        # Windows: Copy DLLs for plugin formats
        message(STATUS "Copying Windows runtime DLLs for ${target}")

        # Set DLL paths
        set(GPAC_DLL        "${CMAKE_SOURCE_DIR}/third_party/gpac/lib/Windows/${CMAKE_BUILD_TYPE}/libgpac.dll")
        set(CRYPTOMD_DLL    "${CMAKE_SOURCE_DIR}/third_party/gpac/lib/Windows/${CMAKE_BUILD_TYPE}/libcryptoMD.dll")
        set(LIBSSLMD_DLL    "${CMAKE_SOURCE_DIR}/third_party/gpac/lib/Windows/${CMAKE_BUILD_TYPE}/libsslMD.dll")  
        set(IAMF_TOOLS_DLL  "${CMAKE_SOURCE_DIR}/third_party/iamftools/lib/Windows/${CMAKE_BUILD_TYPE}/iamf_tools.dll")
        set(OPENSVC_DECODER "${CMAKE_SOURCE_DIR}/third_party/OpenSVC/lib/Windows/${CMAKE_BUILD_TYPE}/OpenSVCDecoder.dll")
        string(TOLOWER "${CMAKE_CFG_INTDIR}" ZMQ_CFG)
        set(ZMQ_DLL         "${CMAKE_BINARY_DIR}/_deps/zeromq-build/bin/${ZMQ_CFG}/libzmq-v143-mt$<$<CONFIG:Debug>:-gd>-4_3_6.dll")
        
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
                COMMAND ${CMAKE_COMMAND} -E copy_if_different ${GPAC_DLL} "${PLUGIN_BINARY_DIR}/"
                COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CRYPTOMD_DLL} "${PLUGIN_BINARY_DIR}/"
                COMMAND ${CMAKE_COMMAND} -E copy_if_different ${LIBSSLMD_DLL} "${PLUGIN_BINARY_DIR}/"
                COMMAND ${CMAKE_COMMAND} -E copy_if_different ${IAMF_TOOLS_DLL} "${PLUGIN_BINARY_DIR}/"
                COMMAND ${CMAKE_COMMAND} -E copy_if_different ${ZMQ_DLL} "${PLUGIN_BINARY_DIR}/"
                COMMENT "Copied DLLs to ${PLUGIN_BINARY_DIR}"
        )

        # Copy DLLs to Debug folder in build directory for use by the JUCE VST3 tool
        # The tool runs from this directory and runs the plugins, so the DLLs need to be here as well
        set(VST3_SIGNING_DIR "${CMAKE_CURRENT_BINARY_DIR}/${BUILD_LIB_DIR}")
        add_custom_command(TARGET ${target} PRE_BUILD
                COMMAND ${CMAKE_COMMAND} -E make_directory "${VST3_SIGNING_DIR}"
                COMMAND ${CMAKE_COMMAND} -E copy_if_different ${GPAC_DLL} "${VST3_SIGNING_DIR}/"
                COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CRYPTOMD_DLL} "${VST3_SIGNING_DIR}/"
                COMMAND ${CMAKE_COMMAND} -E copy_if_different ${LIBSSLMD_DLL} "${VST3_SIGNING_DIR}/"
                COMMAND ${CMAKE_COMMAND} -E copy_if_different ${IAMF_TOOLS_DLL} "${VST3_SIGNING_DIR}/"
                COMMAND ${CMAKE_COMMAND} -E copy_if_different ${ZMQ_DLL} "${VST3_SIGNING_DIR}/"
                COMMENT "Copied DLLs to ${VST3_SIGNING_DIR}"
        )
    endif()
endfunction()