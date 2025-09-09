#!/bin/bash

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

# Exit on any error
set -e

# Configuration
IAMF_TOOLS_REPO="https://github.com/AOMediaCodec/iamf-tools.git"
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
ROOT_DIR="$(dirname "$SCRIPT_DIR")"
TEMP_DIR="$SCRIPT_DIR/iamf_tools_temp"
DEST_LIB_DIR="$ROOT_DIR/lib"
DEST_PROTO_DIR="$ROOT_DIR/iamf/cli/proto"
DEST_INCLUDE_DIR="$ROOT_DIR/iamf/include/iamf_tools"

# Create temporary directory and clone repository
echo "Cloning iamf-tools repository..."
rm -rf "$TEMP_DIR"
git clone "$IAMF_TOOLS_REPO" "$TEMP_DIR"
cd "$TEMP_DIR"

# Get the current commit hash
COMMIT_HASH=$(git rev-parse HEAD)

# Modify the BUILD file to add shared library target
echo "Adding shared library target..."
cat >> iamf/include/iamf_tools/BUILD << 'EOL'
# Shared library exporting all cc_libraries in this file
cc_shared_library(
    name = "iamf_tools",
    deps = [
        ":iamf_decoder_factory",
        ":iamf_decoder_interface",
        ":iamf_encoder_factory",
        ":iamf_encoder_interface",
        ":iamf_tools_api_types",
        ":iamf_tools_encoder_api_types"
    ],
    visibility = ["//visibility:public"],
)
EOL

# Build the shared library
echo "Building shared library..."
bazel build --copt="-g" --strip="never" //iamf/include/iamf_tools:iamf_tools --macos_minimum_os=14 --spawn_strategy=standalone --cxxopt="-std=c++20"

# Create destination directories if they don't exist
mkdir -p "$DEST_LIB_DIR" "$DEST_PROTO_DIR" "$DEST_INCLUDE_DIR"

# Copy the built library
echo "Copying library files..."
cp bazel-bin/iamf/include/iamf_tools/libiamf_tools.dylib "$DEST_LIB_DIR/"

# Copy proto files
echo "Copying and updating proto files..."
# Create proto directory if it doesn't exist
mkdir -p "$DEST_PROTO_DIR"
# Save CMakeLists.txt if it exists
if [ -f "$DEST_PROTO_DIR/CMakeLists.txt" ]; then
    CMAKELISTS_BACKUP=$(mktemp)
    cp "$DEST_PROTO_DIR/CMakeLists.txt" "$CMAKELISTS_BACKUP"
fi
# Use a temporary directory for proto files
PROTO_TEMP_DIR=$(mktemp -d)
cp "$TEMP_DIR/iamf/cli/proto/"*.proto "$PROTO_TEMP_DIR/"
cd "$PROTO_TEMP_DIR"
# Update import paths in proto files
for file in *.proto; do
    sed -i '' 's|import "iamf/cli/proto/|import "|g' "$file"
done
# Copy the modified proto files to the final destination
cp *.proto "$DEST_PROTO_DIR/"
# Restore CMakeLists.txt if it was backed up
if [ -f "$CMAKELISTS_BACKUP" ]; then
    cp "$CMAKELISTS_BACKUP" "$DEST_PROTO_DIR/CMakeLists.txt"
    rm "$CMAKELISTS_BACKUP"
fi
# Clean up the temporary proto directory
rm -rf "$PROTO_TEMP_DIR"

# Go back to the temp dir to copy headers
cd "$TEMP_DIR"

# Copy header files
echo "Copying header files..."
cp iamf/include/iamf_tools/*.h "$DEST_INCLUDE_DIR/"

# Go back to the script directory to ensure cleanup paths are correct
cd "$SCRIPT_DIR"

# Fix rpaths in the dylib
echo "Fixing rpaths in dylib..."
install_name_tool -add_rpath "@rpath/third_party/iamftools/lib/" "$DEST_LIB_DIR/libiamf_tools.dylib"
install_name_tool -add_rpath "@rpath/third_party/iamftools/lib/libiamf_tools.dylib" "$DEST_LIB_DIR/libiamf_tools.dylib"
install_name_tool -id "@rpath/third_party/iamftools/lib/libiamf_tools.dylib" "$DEST_LIB_DIR/libiamf_tools.dylib"

# Update README.md with new commit hash
echo "Updating README.md..."
sed -i '' "2s|.*|- Commit: https://github.com/AOMediaCodec/iamf-tools/commit/$COMMIT_HASH|" "$ROOT_DIR/README.md"

# Final cleanup
echo "Cleaning up temporary files..."
rm -rf "$TEMP_DIR"

echo "Update completed successfully!"
