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

    if (APPLE)
        set(DEST_ROOT "${plugin_path}/Contents/Resources")

        add_custom_command(TARGET ${target} PRE_BUILD
                COMMAND ${CMAKE_COMMAND} -E make_directory "${DEST_ROOT}"
                COMMAND ${CMAKE_COMMAND} -E make_directory "${DEST_ROOT}/third_party/gpac/lib"
                COMMAND ${CMAKE_COMMAND} -E make_directory "${DEST_ROOT}/third_party/iamftools/lib"
                COMMAND ${CMAKE_COMMAND} -E make_directory "${DEST_ROOT}/third_party/obr/lib"
                COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:vendored_gpac>" "${DEST_ROOT}/third_party/gpac/lib/"
                COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:vendored_iamf_tools>" "${DEST_ROOT}/third_party/iamftools/lib/"
                COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:vendored_obr>" "${DEST_ROOT}/third_party/obr/lib/"
                COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:libzmq>" "${DEST_ROOT}/"
                COMMAND ${CMAKE_COMMAND} -E create_symlink "$<TARGET_FILE_NAME:libzmq>" "${DEST_ROOT}/libzmq.5.dylib"
                COMMAND ${CMAKE_COMMAND} -E create_symlink "libzmq.5.dylib" "${DEST_ROOT}/libzmq.dylib"
        )

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

        set(COPY_COMMANDS COMMAND ${CMAKE_COMMAND} -E make_directory "${DEST_ROOT}")
        foreach (_dep IN LISTS ECLIPSA_VENDORED_LIBS)
            list(APPEND COPY_COMMANDS
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:${_dep}>" "${DEST_ROOT}/")
            target_link_options(${target} PRIVATE "/DELAYLOAD:$<TARGET_FILE_NAME:${_dep}>")
        endforeach ()
        add_custom_command(TARGET ${target} PRE_BUILD ${COPY_COMMANDS})

        set(VST3_SIGNING_DIR "${CMAKE_CURRENT_BINARY_DIR}/${BUILD_LIB_DIR}")
        set(SIGNING_COMMANDS COMMAND ${CMAKE_COMMAND} -E make_directory "${VST3_SIGNING_DIR}")
        foreach (_dep IN LISTS ECLIPSA_VENDORED_LIBS)
            list(APPEND SIGNING_COMMANDS
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:${_dep}>" "${VST3_SIGNING_DIR}/")
        endforeach ()
        add_custom_command(TARGET ${target} PRE_BUILD ${SIGNING_COMMANDS})
    endif ()

endfunction()