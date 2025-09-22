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

# cmake/copy_resources.cmake
function(copy_resources target plugin_path)
    # Define paths to required libraries
    set(LIB_OBR_PATH "${CMAKE_SOURCE_DIR}/third_party/obr/lib/obr.dylib")
    set(LIB_RENDERER_ENCODER_PATH "${CMAKE_SOURCE_DIR}/third_party/iamftools/lib/libiamf_tools.dylib")
    set(LIB_ZMQ_5_2_6_PATH "${CMAKE_BINARY_DIR}/_deps/zeromq-build/lib/libzmq.5.2.6.dylib")
    set(LIB_ZMQ_5_SYMLINK "${CMAKE_BINARY_DIR}/_deps/zeromq-build/lib/libzmq.5.dylib")
    set(LIB_ZMQ_SYMLINK "${CMAKE_BINARY_DIR}/_deps/zeromq-build/lib/libzmq.dylib")

    # Set Resources directory and external subdirectories
    set(RESOURCES_DIR "${plugin_path}/Contents/Resources")
    set(EXTERNAL_IAMF_DIR "${RESOURCES_DIR}/third_party/iamftools/lib")
    set(EXTERNAL_OBR_DIR "${RESOURCES_DIR}/third_party/obr/lib")

    # Create necessary directories
    file(MAKE_DIRECTORY ${EXTERNAL_IAMF_DIR})
    file(MAKE_DIRECTORY ${EXTERNAL_OBR_DIR})

    # Copy libraries to the appropriate directories
    add_custom_command(TARGET ${target} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy ${LIB_OBR_PATH} ${EXTERNAL_OBR_DIR}/obr.dylib
        COMMAND ${CMAKE_COMMAND} -E copy ${LIB_RENDERER_ENCODER_PATH} ${EXTERNAL_IAMF_DIR}/libiamf_tools.dylib
        COMMAND ${CMAKE_COMMAND} -E copy ${LIB_ZMQ_5_2_6_PATH} ${RESOURCES_DIR}/libzmq.5.2.6.dylib

        # Create symbolic links for libzmq
        COMMAND ${CMAKE_COMMAND} -E create_symlink libzmq.5.2.6.dylib ${RESOURCES_DIR}/libzmq.5.dylib
        COMMAND ${CMAKE_COMMAND} -E create_symlink libzmq.5.dylib ${RESOURCES_DIR}/libzmq.dylib
    )
endfunction()