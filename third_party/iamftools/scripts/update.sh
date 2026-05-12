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

set -e

IAMF_TOOLS_REPO="https://github.com/AOMediaCodec/iamf-tools.git"
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
ROOT_DIR="$(dirname "$SCRIPT_DIR")"
TEMP_DIR="$SCRIPT_DIR/iamf_tools_temp"
DEST_LIB_DIR="$ROOT_DIR/lib"
DEST_PROTO_DIR="$ROOT_DIR/iamf/cli/proto"
DEST_INCLUDE_DIR="$ROOT_DIR/iamf/include/iamf_tools"

# Use the provided version tag, or fetch the latest release from GitHub.
if [ -n "$1" ]; then
    VERSION="$1"
else
    echo "Fetching latest iamf-tools release..."
    VERSION=$(curl -s "https://api.github.com/repos/AOMediaCodec/iamf-tools/releases/latest" \
        | python3 -c "import json,sys; print(json.load(sys.stdin)['tag_name'])")
    echo "Latest release: $VERSION"
fi

# Clone the repo. For a tag or branch, use a fast shallow clone.
# For a raw commit hash, do a treeless clone then check out the commit.
echo "Cloning iamf-tools $VERSION..."
rm -rf "$TEMP_DIR"
if [[ "$VERSION" =~ ^[0-9a-f]{40}$ ]]; then
    git clone --filter=blob:none "$IAMF_TOOLS_REPO" "$TEMP_DIR"
    git -C "$TEMP_DIR" checkout "$VERSION"
else
    git clone --branch "$VERSION" --depth 1 "$IAMF_TOOLS_REPO" "$TEMP_DIR"
fi
COMMIT_HASH=$(git -C "$TEMP_DIR" rev-parse HEAD)

# Append shared library target to BUILD file.
echo "Adding shared library target..."
cat >> "$TEMP_DIR/iamf/include/iamf_tools/BUILD" << 'EOL'

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

# Build the shared library.
echo "Building shared library..."
(cd "$TEMP_DIR" && bazel build \
    --copt="-g" \
    --strip="never" \
    --macos_minimum_os=14 \
    --spawn_strategy=standalone \
    --cxxopt="-std=c++20" \
    //iamf/include/iamf_tools:iamf_tools)

mkdir -p "$DEST_LIB_DIR" "$DEST_PROTO_DIR" "$DEST_INCLUDE_DIR"

# Copy the built library.
echo "Copying library files..."
cp "$TEMP_DIR/bazel-bin/iamf/include/iamf_tools/libiamf_tools.dylib" "$DEST_LIB_DIR/"

# Copy and patch proto files, preserving any existing CMakeLists.txt.
echo "Copying and updating proto files..."
CMAKELISTS_BACKUP=""
if [ -f "$DEST_PROTO_DIR/CMakeLists.txt" ]; then
    CMAKELISTS_BACKUP=$(mktemp)
    cp "$DEST_PROTO_DIR/CMakeLists.txt" "$CMAKELISTS_BACKUP"
fi

PROTO_TEMP_DIR=$(mktemp -d)
cp "$TEMP_DIR/iamf/cli/proto/"*.proto "$PROTO_TEMP_DIR/"
for file in "$PROTO_TEMP_DIR"/*.proto; do
    sed -i '' 's|import "iamf/cli/proto/|import "|g' "$file"
done
cp "$PROTO_TEMP_DIR/"*.proto "$DEST_PROTO_DIR/"
rm -rf "$PROTO_TEMP_DIR"

if [ -n "$CMAKELISTS_BACKUP" ]; then
    cp "$CMAKELISTS_BACKUP" "$DEST_PROTO_DIR/CMakeLists.txt"
    rm "$CMAKELISTS_BACKUP"
fi

# Copy header files.
echo "Copying header files..."
cp "$TEMP_DIR/iamf/include/iamf_tools/"*.h "$DEST_INCLUDE_DIR/"

# Fix rpaths in the dylib.
echo "Fixing rpaths in dylib..."
install_name_tool -add_rpath "@rpath/third_party/iamftools/lib/" "$DEST_LIB_DIR/libiamf_tools.dylib"
install_name_tool -add_rpath "@rpath/third_party/iamftools/lib/libiamf_tools.dylib" "$DEST_LIB_DIR/libiamf_tools.dylib"
install_name_tool -id "@rpath/third_party/iamftools/lib/libiamf_tools.dylib" "$DEST_LIB_DIR/libiamf_tools.dylib"

# Update commit and version in README.
echo "Updating README.md..."
VERSION_NUM="${VERSION#v}"
sed -i '' "s|^- Commit:.*|- Commit: https://github.com/AOMediaCodec/iamf-tools/commit/$COMMIT_HASH|" "$ROOT_DIR/README.md"
sed -i '' "s|^- Version:.*|- Version: $VERSION_NUM|" "$ROOT_DIR/README.md"

echo "Cleaning up..."
rm -rf "$TEMP_DIR"

echo "Updated to $VERSION ($COMMIT_HASH) successfully!"
