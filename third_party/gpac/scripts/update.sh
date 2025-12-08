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
GPAC_REPO="https://github.com/gpac/gpac.git"
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
ROOT_DIR="$(dirname "$SCRIPT_DIR")"
TEMP_DIR="$SCRIPT_DIR/gpac_temp"
DEST_LIB_DIR="$ROOT_DIR/lib"
DEST_INCLUDE_DIR="$ROOT_DIR/include"
INSTALL_DIR="$TEMP_DIR/install"

# Create temporary directory and clone repository
echo "Cloning gpac repository..."
rm -rf "$TEMP_DIR"
git clone "$GPAC_REPO" "$TEMP_DIR"
cd "$TEMP_DIR"

# Get the current commit hash
COMMIT_HASH=$(git rev-parse HEAD)

# Configure the build
# Write a list of extra libs to disable
CFG_CMD="./configure --prefix="$INSTALL_DIR" --static-modules --isomedia-only --disable-rmtws"
EXTRA_LIBS="zlib opensvc openhevc platinum freetype ssl jpeg openjpeg png mad a52 xvid faad ffmpeg freenect vorbis theora nghttp2 ngtcp2 nghttp3 oss dvb4linux alsa pulseaudio jack directfb hid lzma tinygl vtb ogg sdl caption mpeghdec libcaca curl"
LIBS="compositor fin fout mp4mx mp4dmx reframer rfav1"
# For each library in extra libs, append '--use-<lib>=no' to the configure command
for lib in $EXTRA_LIBS; do
    CFG_CMD+=" --use-$lib=no"
done
# For each library in libs, append '--enable-<lib>' to the configure command
for lib in $LIBS; do
    CFG_CMD+=" --enable-$lib"
done

echo "Configuring gpac build..."
$CFG_CMD

# Build and install the library
echo "Building gpac..."
make -j8

echo "Installing gpac library..."
make install-lib

# Create destination directories if they don't exist
mkdir -p "$DEST_LIB_DIR" "$DEST_INCLUDE_DIR"

# Copy the built library
echo "Copying library files..."
cp "$INSTALL_DIR/lib/libgpac.dylib" "$DEST_LIB_DIR/"

# Fix rpaths in the dylib
echo "Fixing rpaths in dylib..."
install_name_tool -add_rpath "@rpath/third_party/gpac/lib/" "$DEST_LIB_DIR/libgpac.dylib"
install_name_tool -add_rpath "@rpath/third_party/gpac/lib/libgpac.dylib" "$DEST_LIB_DIR/libgpac.dylib"
install_name_tool -id "@rpath/third_party/gpac/lib/libgpac.dylib" "$DEST_LIB_DIR/libgpac.dylib"

# Copy header files
echo "Copying header files..."
rm -rf "$DEST_INCLUDE_DIR/gpac"
cp -r "$INSTALL_DIR/include/gpac" "$DEST_INCLUDE_DIR/"

# Update README.md with new commit hash
echo "Updating README.md..."
sed -i '' "1s|.*|- Compiled from: https://github.com/gpac/gpac/tree/$COMMIT_HASH|" "$ROOT_DIR/README.md"

# Go back to the script directory to ensure cleanup paths are correct
cd "$SCRIPT_DIR"

# Final cleanup
echo "Cleaning up temporary files..."
rm -rf "$TEMP_DIR"

echo "Update completed successfully!"
