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

    set(_BASE "$<TARGET_FILE_DIR:${target}>")

    # Determine destination based on platform
    if (TARGET vendored_obr)
        # macOS: bundle structure
        set(DEST_ROOT "$<TARGET_BUNDLE_CONTENT_DIR:${target}>/Resources")
        set(DEST_IAMF "${DEST_ROOT}/third_party/iamftools/lib")
        set(DEST_GPAC "${DEST_ROOT}/third_party/gpac/lib")
        set(DEST_OBR "${DEST_ROOT}/third_party/obr/lib")
    else ()
        # Windows: flat structure based on target type
        if ("${target}" MATCHES ".*_VST3$")
            set(DEST_ROOT "${_BASE}/Contents/x86_64-win")
        elseif ("${target}" MATCHES ".*_AAX$")
            set(DEST_ROOT "${_BASE}/Contents/x64")
        else ()
            set(DEST_ROOT "${_BASE}")
        endif ()
        set(DEST_IAMF "${DEST_ROOT}")
        set(DEST_GPAC "${DEST_ROOT}")
    endif ()

    # Delay-load (Windows only)
    if (DEFINED ECLIPSA_DELAYLOAD_LIBS)
        foreach (_lib ${ECLIPSA_DELAYLOAD_LIBS})
            target_link_options(${target} PRIVATE "/DELAYLOAD:$<TARGET_FILE_NAME:${_lib}>")
        endforeach ()
    endif ()

    # Create directories
    add_custom_command(TARGET ${target} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory "${DEST_ROOT}"
            COMMAND ${CMAKE_COMMAND} -E make_directory "${DEST_IAMF}"
            COMMAND ${CMAKE_COMMAND} -E make_directory "${DEST_GPAC}"
            COMMENT "Deploying runtime deps to ${target}"
    )

    # Copy vendored libs (uses ECLIPSA_VENDORED_LIBS from prebuiltLibs)
    foreach (_lib ${ECLIPSA_VENDORED_LIBS})
        if (TARGET ${_lib})
            if ("${_lib}" STREQUAL "vendored_gpac")
                set(_dest "${DEST_GPAC}")
            elseif ("${_lib}" STREQUAL "vendored_iamf_tools")
                set(_dest "${DEST_IAMF}")
            elseif ("${_lib}" STREQUAL "vendored_obr")
                set(_dest "${DEST_OBR}")
                add_custom_command(TARGET ${target} POST_BUILD APPEND
                        COMMAND ${CMAKE_COMMAND} -E make_directory "${DEST_OBR}"
                )
            else ()
                set(_dest "${DEST_ROOT}")
            endif ()

            add_custom_command(TARGET ${target} POST_BUILD APPEND
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:${_lib}>" "${_dest}/"
            )
        endif ()
    endforeach ()

    # macOS symlinks for zmq
    if (TARGET vendored_obr)
        add_custom_command(TARGET ${target} POST_BUILD APPEND
                COMMAND ${CMAKE_COMMAND} -E create_symlink "$<TARGET_FILE_NAME:libzmq>" "${DEST_ROOT}/libzmq.5.dylib"
                COMMAND ${CMAKE_COMMAND} -E create_symlink "libzmq.5.dylib" "${DEST_ROOT}/libzmq.dylib"
        )
    endif ()

endfunction()