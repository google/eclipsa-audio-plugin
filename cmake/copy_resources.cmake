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

    # --- 1. Define Destinations ---
    if (APPLE)
        set(DEST_ROOT "${plugin_path}/Contents/Resources")
        set(DEST_IAMF "${DEST_ROOT}/third_party/iamftools/lib")
        set(DEST_OBR "${DEST_ROOT}/third_party/obr/lib")
        set(DEST_GPAC "${DEST_ROOT}/third_party/gpac/lib")
    elseif (WIN32)
        if ("${target}" MATCHES ".*_VST3$")
            set(DEST_ROOT "${plugin_path}/Contents/x86_64-win")
        elseif ("${target}" MATCHES ".*_AAX$")
            set(DEST_ROOT "${plugin_path}/Contents/x64")
        elseif ("${target}" MATCHES ".*_Standalone$")
            set(DEST_ROOT "${plugin_path}")
        else ()
            message(WARNING "Unknown plugin format: ${target}")
            return()
        endif ()

        set(DEST_IAMF "${DEST_ROOT}")
        set(DEST_GPAC "${DEST_ROOT}")
        # DEST_OBR is not needed on Windows
    endif ()

    # --- 2. Windows: Apply Delay Load Flags ---
    if (WIN32)
        target_link_options(${target} PRIVATE
                "/DELAYLOAD:$<TARGET_FILE_NAME:vendored_gpac>"
                "/DELAYLOAD:$<TARGET_FILE_NAME:vendored_iamf_tools>"
                "/DELAYLOAD:$<TARGET_FILE_NAME:vendored_opensvc>"
                "/DELAYLOAD:$<TARGET_FILE_NAME:libzmq>"
                "/DELAYLOAD:$<TARGET_FILE_NAME:vendored_gpac_crypto>"
                "/DELAYLOAD:$<TARGET_FILE_NAME:vendored_gpac_ssl>"
        )
        set(DELAYLOAD_DLLS "" PARENT_SCOPE)
    endif ()

    # --- 3. Copy Commands (Grouped by Platform to avoid evaluation crashes) ---

    # A. Commands Common to BOTH platforms
    set(COPY_COMMANDS
            COMMAND ${CMAKE_COMMAND} -E make_directory "${DEST_ROOT}"
            COMMAND ${CMAKE_COMMAND} -E make_directory "${DEST_IAMF}"
            COMMAND ${CMAKE_COMMAND} -E make_directory "${DEST_GPAC}"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:vendored_gpac>" "${DEST_GPAC}/"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:vendored_iamf_tools>" "${DEST_IAMF}/"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:libzmq>" "${DEST_ROOT}/"
    )

    # B. Windows-Only Commands
    if (WIN32)
        list(APPEND COPY_COMMANDS
                COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:vendored_opensvc>" "${DEST_ROOT}/"
                COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:vendored_gpac_crypto>" "${DEST_ROOT}/"
                COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:vendored_gpac_ssl>" "${DEST_ROOT}/"
        )
    endif ()

    # C. Mac-Only Commands
    if (APPLE)
        list(APPEND COPY_COMMANDS
                COMMAND ${CMAKE_COMMAND} -E make_directory "${DEST_OBR}"
                COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:vendored_obr>" "${DEST_OBR}/"
        )
    endif ()

    # Apply the aggregated commands
    add_custom_command(TARGET ${target} POST_BUILD
            ${COPY_COMMANDS}
            COMMENT "Deploying resources to ${DEST_ROOT}"
    )

    # --- 4. macOS ZMQ Symlinks ---
    if (APPLE)
        add_custom_command(TARGET ${target} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E create_symlink "$<TARGET_FILE_NAME:libzmq>" "${DEST_ROOT}/libzmq.5.dylib"
                COMMAND ${CMAKE_COMMAND} -E create_symlink "libzmq.5.dylib" "${DEST_ROOT}/libzmq.dylib"
        )
    endif ()

    # --- 5. Helper Tool Copy (Windows Only) ---
    if (WIN32)
        set(VST3_SIGNING_DIR "${CMAKE_CURRENT_BINARY_DIR}/${BUILD_LIB_DIR}")
        add_custom_command(TARGET ${target} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E make_directory "${VST3_SIGNING_DIR}"
                COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:vendored_gpac>" "${VST3_SIGNING_DIR}/"
                COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:vendored_iamf_tools>" "${VST3_SIGNING_DIR}/"
                COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:libzmq>" "${VST3_SIGNING_DIR}/"
                COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:vendored_opensvc>" "${VST3_SIGNING_DIR}/"
                COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:vendored_gpac_crypto>" "${VST3_SIGNING_DIR}/"
                COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:vendored_gpac_ssl>" "${VST3_SIGNING_DIR}/"
        )
    endif ()
endfunction()