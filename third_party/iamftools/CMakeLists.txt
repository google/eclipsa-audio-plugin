add_library(iamftools INTERFACE)
target_compile_options(iamftools INTERFACE "-w") # Silence is golden
target_include_directories(iamftools INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/iamf/include/iamf_tools")

add_library(iamftools_dylib SHARED IMPORTED)
set_target_properties(iamftools_dylib PROPERTIES
  IMPORTED_LOCATION         "${CMAKE_CURRENT_LIST_DIR}/lib/libiamf_renderer_encoder.dylib"
  IMPORTED_LOCATION_DEBUG   "${CMAKE_CURRENT_LIST_DIR}/lib/libiamf_renderer_encoder.dylib"
  IMPORTED_LOCATION_RELEASE "${CMAKE_CURRENT_LIST_DIR}/lib/libiamf_renderer_encoder.dylib"
  IMPORTED_LOCATION_RELWITHDEBINFO "${CMAKE_CURRENT_LIST_DIR}/lib/libiamf_renderer_encoder.dylib"
  IMPORTED_SONAME "libiamf_renderer_encoder.dylib"
  LINKER_LANGUAGE CXX)

# Parse the commit hash
# Set the path to the README.md file
set(README_FILE "${CMAKE_CURRENT_LIST_DIR}/README.md")

file(STRINGS "${README_FILE}" COMMIT_LINE REGEX "Commit: https://github.com/AOMediaCodec/iamf-tools/commit/([a-f0-9]+)")
string(REGEX REPLACE ".*commit/([a-f0-9]+).*" "GitHub/iamf-tools@\\1" COMMIT_HASH "${COMMIT_LINE}")

# Set the path to the header and template file
set(HEADER_FILE "${CMAKE_CURRENT_LIST_DIR}/iamf/include/iamf_tools/commit_hash.h")
set(TEMPLATE_FILE "${CMAKE_CURRENT_LIST_DIR}/iamf/include/iamf_tools/commit_hash.h.in")

# Configure the header file
configure_file(${TEMPLATE_FILE} ${HEADER_FILE} @ONLY)

# Find required protobuf file generate fx
include("${CMAKE_SOURCE_DIR}/third_party/protobuf/cmake/protobuf-generate.cmake")

set(PROTO_FILES
  "iamf/cli/proto/arbitrary_obu.proto"
  "iamf/cli/proto/audio_element.proto"
  "iamf/cli/proto/audio_frame.proto"
  "iamf/cli/proto/codec_config.proto"
  "iamf/cli/proto/encoder_control_metadata.proto"
  "iamf/cli/proto/ia_sequence_header.proto"
  "iamf/cli/proto/mix_presentation.proto"
  "iamf/cli/proto/obu_header.proto"
  "iamf/cli/proto/output_audio_format.proto"
  "iamf/cli/proto/param_definitions.proto"
  "iamf/cli/proto/parameter_block.proto"
  "iamf/cli/proto/parameter_data.proto"
  "iamf/cli/proto/temporal_delimiter.proto"
  "iamf/cli/proto/test_vector_metadata.proto"
  "iamf/cli/proto/user_metadata.proto"
)

set(PROTO_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR})

# Generate protobuf files
add_library(proto-objects OBJECT ${PROTO_FILES})
target_include_directories(proto-objects PUBLIC "${CMAKE_CURRENT_BINARY_DIR}" "${PROTO_BINARY_DIR}/iamf/cli/proto")
target_link_libraries(proto-objects PUBLIC protobuf::libprotobuf)
protobuf_generate(
  TARGET proto-objects
  IMPORT_DIRS "${CMAKE_CURRENT_LIST_DIR}"
  PROTOC_OUT_DIR "${PROTO_BINARY_DIR}"
)

target_link_libraries(iamftools
  INTERFACE
    iamftools_dylib
    proto-objects
)

if(CI_TEST OR INTERNAL_TEST)
  eclipsa_add_test(test_iamf_sanity test/libiamf_test.cpp "iamftools")
endif()