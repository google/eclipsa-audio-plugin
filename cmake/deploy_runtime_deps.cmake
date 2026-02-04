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

function(deploy_runtime_deps target)

    if (APPLE)
        set(DEST_ROOT "$<TARGET_BUNDLE_CONTENT_DIR:${target}>/Resources")
        set(DEST_IAMF "${DEST_ROOT}/third_party/iamftools/lib")
        set(DEST_GPAC "${DEST_ROOT}/third_party/gpac/lib")
        set(DEST_OBR "${DEST_ROOT}/third_party/obr/lib")
    elseif (WIN32)
        # Mirror your existing layout logic, but anchored to the real target output dir
        # JUCE often outputs .../Eclipsa Audio Element Plugin.vst3/Contents/x86_64-win
        # so we *derive* the correct folder rather than hardcoding the artefacts path.
        set(_BASE "$<TARGET_FILE_DIR:${target}>")

        # Your old logic used target name suffixes; keep that just for the destination
        if ("${target}" MATCHES ".*_VST3$")
            set(DEST_ROOT "${_BASE}/Contents/x86_64-win")
        elseif ("${target}" MATCHES ".*_AAX$")
            set(DEST_ROOT "${_BASE}/Contents/x64")
        elseif ("${target}" MATCHES ".*_Standalone$")
            set(DEST_ROOT "${_BASE}")
        else ()
            set(DEST_ROOT "${_BASE}")
        endif ()

        set(DEST_IAMF "${DEST_ROOT}")
        set(DEST_GPAC "${DEST_ROOT}")
    endif ()

    # Delay-load flags stay (Windows only)
    if (WIN32)
        target_link_options(${target} PRIVATE
                "/DELAYLOAD:$<TARGET_FILE_NAME:vendored_gpac>"
                "/DELAYLOAD:$<TARGET_FILE_NAME:vendored_iamf_tools>"
                "/DELAYLOAD:$<TARGET_FILE_NAME:vendored_opensvc>"
                "/DELAYLOAD:$<TARGET_FILE_NAME:libzmq>"
                "/DELAYLOAD:$<TARGET_FILE_NAME:vendored_gpac_crypto>"
                "/DELAYLOAD:$<TARGET_FILE_NAME:vendored_gpac_ssl>"
        )
    endif ()

    # Copy deps
    add_custom_command(TARGET ${target} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory "${DEST_ROOT}"
            COMMAND ${CMAKE_COMMAND} -E make_directory "${DEST_IAMF}"
            COMMAND ${CMAKE_COMMAND} -E make_directory "${DEST_GPAC}"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:vendored_gpac>" "${DEST_GPAC}/"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:vendored_iamf_tools>" "${DEST_IAMF}/"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:libzmq>" "${DEST_ROOT}/"
            COMMENT "Deploying runtime deps to ${target}"
    )

    if (WIN32)
        add_custom_command(TARGET ${target} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:vendored_opensvc>" "${DEST_ROOT}/"
                COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:vendored_gpac_crypto>" "${DEST_ROOT}/"
                COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:vendored_gpac_ssl>" "${DEST_ROOT}/"
        )
    endif ()

    if (APPLE)
        add_custom_command(TARGET ${target} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E make_directory "${DEST_OBR}"
                COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:vendored_obr>" "${DEST_OBR}/"
                COMMAND ${CMAKE_COMMAND} -E create_symlink "$<TARGET_FILE_NAME:libzmq>" "${DEST_ROOT}/libzmq.5.dylib"
                COMMAND ${CMAKE_COMMAND} -E create_symlink "libzmq.5.dylib" "${DEST_ROOT}/libzmq.dylib"
        )
    endif ()

endfunction()

