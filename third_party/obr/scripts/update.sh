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
OBR_REPO="https://github.com/google/obr.git"
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
ROOT_DIR="$(dirname "$SCRIPT_DIR")"
TEMP_DIR="$SCRIPT_DIR/obr_temp"
DEST_LIB_DIR="$ROOT_DIR/lib"
DEST_INCLUDE_DIR="$ROOT_DIR/obr"

# Create temporary directory and clone repository
echo "Cloning OBR repository..."
rm -rf "$TEMP_DIR"
git clone "$OBR_REPO" "$TEMP_DIR"
cd "$TEMP_DIR"

# Get the current commit hash
COMMIT_HASH=$(git rev-parse HEAD)

# Update MODULE.bazel with correct abseil version
echo "Updating abseil dependency..."
sed -i '' $'/name = "abseil-cpp"/{n;s/version = \\"[^\\"]*\\"/version = \\"20250512.1\\"/;}' MODULE.bazel

# Modify the BUILD file to add shared library target
echo "Adding shared library target..."
cat >> obr/renderer/BUILD << 'EOL'
cc_binary(
    name = "obr",
    deps = [
        ":obr_impl",
    ],
    linkshared = True,
)
EOL

# Update the obr_impl target to include required flags
echo "Updating obr_impl target..."
sed -i '' '/name = "obr_impl"/,/^)/ {
    /deps = \[/,/\]/ {
        /\]/a\
    linkstatic = False,\
    alwayslink = True,
    }
}' obr/renderer/BUILD

# Build the shared library
echo "Building shared library..."
bazel build --compilation_mode=opt //obr/renderer:obr

# Create destination directories if they don't exist
# Create destination directories with user write permissions
mkdir -p "$DEST_LIB_DIR" "$DEST_INCLUDE_DIR"
chmod 755 "$DEST_LIB_DIR" "$DEST_INCLUDE_DIR"
chmod 755 "$ROOT_DIR/lib"  # Ensure parent directory is also writable

# Copy the built library
echo "Copying library files..."
cp bazel-bin/obr/renderer/libobr.dylib "$DEST_LIB_DIR/obr.dylib"

# Copy header files
echo "Copying header files..."
cp -r obr/* "$DEST_INCLUDE_DIR/"

# Clean up non-header files
echo "Cleaning up non-header files..."
"$SCRIPT_DIR/clean_directory.sh" "$DEST_INCLUDE_DIR"

# Fix rpaths in the dylib
echo "Fixing rpaths in dylib..."
install_name_tool -id "@rpath/third_party/obr/lib/obr.dylib" "$DEST_LIB_DIR/obr.dylib"

# Verify the rpath was updated correctly
echo "Verifying rpath..."
otool -l "$DEST_LIB_DIR/obr.dylib" | grep obr.dylib

# Update README.md with new commit hash
echo "Updating README.md..."
sed -i '' "2s|.*|- Commit: https://github.com/google/obr/commit/$COMMIT_HASH|" "$ROOT_DIR/README.md"

# Final cleanup
echo "Cleaning up temporary files..."
rm -rf "$TEMP_DIR"

echo "Update completed successfully!"
