#!/bin/bash -eu
#
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

# USAGE GUIDE:
# This script builds installers for AAX plugins or VST3 plugins separately:
# --format=aax: AAX plugins only (default) - installs for all users only
# --format=vst3: VST3 plugins only - allows user to choose installation location
# -- format=au: AU plugins only - allows user to choose installation location
#
# AAX plugins: Uses wraptool if present, otherwise uses ad-hoc signing
# VST3 plugins: Uses standard Apple code signing
# AU plugins: Uses standard Apple code signing
#
# For official code signing and notarization:
# 1. Create a keychain profile: 
#    xcrun notarytool store-credentials "ProfileName" --apple-id "AppleID" --team-id "TeamID"
# 2. Set environment variables for signing identities

# Parse command line arguments
PLUGIN_FORMAT="aax"  # Default to AAX

for arg in "$@"; do
    case $arg in
        --format=*)
        PLUGIN_FORMAT="${arg#*=}"
        shift
        ;;
    esac
done

# Validate plugin format option
case $PLUGIN_FORMAT in
    aax|vst3|au)
        echo "Building for plugin format(s): $PLUGIN_FORMAT"
        ;;
    *)
        echo "Error: Invalid format option. Use --format=aax, --format=vst3, or --format=au"
        exit 1
        ;;
esac

case "$PLUGIN_FORMAT" in
    aax)
        FORMAT_SUFFIX="AAX"
        DISTRIBUTION_XML="./distribution_aax.xml"
        ;;
    vst3)
        FORMAT_SUFFIX="VST3"
        DISTRIBUTION_XML="./distribution_vst3.xml"
        ;;
    au)
        FORMAT_SUFFIX="AU"
        DISTRIBUTION_XML="./distribution_au.xml"
        ;;
esac

# ============================
# CONFIGURATION VARIABLES 
# ============================

# Set up logging
LOGFILE="./package_$(date +%Y%m%d_%H%M%S)_${FORMAT_SUFFIX}.log"
exec > >(tee -a "$LOGFILE") 2>&1

log() {
    local timestamp
    timestamp=$(date "+%Y-%m-%d %H:%M:%S")
    echo -e "[$timestamp] $1"
}

log_section() {
    echo ""
    log "================================================================"
    log "$1"
    log "================================================================"
    echo ""
}

# Load configuration from config file if it exists
CONFIG_FILE="./package_config.sh"
if [ -f "$CONFIG_FILE" ]; then
    source "$CONFIG_FILE"
    log "Configuration loaded from $CONFIG_FILE"
fi

# Plugin Build Config
BUILD_CONFIG="${ECLIPSA_BUILD_CONFIG:-Release}"

# Basic branding - use environment variables with defaults
COMPANY_NAME="${ECLIPSA_COMPANY_NAME:-Eclipsa Project}"
COPYRIGHT_YEAR="${ECLIPSA_COPYRIGHT_YEAR:-2025}"
BUNDLE_IDENTIFIER="${ECLIPSA_BUNDLE_IDENTIFIER:-com.eclipsaproject.plugins}"
VERSION="${ECLIPSA_VERSION:-1.0.0}"
DOCS_URL="${ECLIPSA_DOCS_URL:-https://docs.eclipsaapp.com}"

# DMG appearance
DMG_TITLE="${ECLIPSA_DMG_TITLE:-Eclipsa Plugins Installer}"
DMG_WINDOW_SIZE="${ECLIPSA_DMG_WINDOW_SIZE:-{300, 100, 1000, 598}}" # {x, y, width, height}
APP_POSITION="${ECLIPSA_APP_POSITION:-{350, 220}}"               # Position of installer app
DOC_POSITION="${ECLIPSA_DOC_POSITION:-{250, 370}}"               # Position of Documentation folder
LICENSE_POSITION="${ECLIPSA_LICENSE_POSITION:-{450, 370}}"           # Position of Licenses folder

# Asset paths
CUSTOM_ICON_FILE="${ECLIPSA_CUSTOM_ICON_FILE:-./custom_installer_icon.png}"
DMG_BACKGROUND_FILE="${ECLIPSA_DMG_BACKGROUND_FILE:-./ECLIPSA_badge_ic_H_4200x952px_GREY-POS@2x.png}"
LICENSE_FILE="${ECLIPSA_LICENSE_FILE:-../LICENSE}"

# For official notarization (can be provided via environment variables)
# DEVELOPER_ID_APPLICATION can be set in environment
# DEVELOPER_ID_INSTALLER can be set in environment
# NOTARIZATION_KEYCHAIN_PROFILE can be set in environment

# ======================================================
# CORE PATHS - NORMALLY DON'T NEED TO MODIFY
# ======================================================

# Define paths to the plugins - AAX, VST3 and AU
# AAX Plugin Paths
AAX_RENDERER_PLUGIN_BINARY="../build/rendererplugin/RendererPlugin_artefacts/$BUILD_CONFIG/AAX/Eclipsa Audio Renderer.aaxplugin/Contents/MacOS/Eclipsa Audio Renderer"
AAX_AUDIOELEMENT_PLUGIN_BINARY="../build/audioelementplugin/AudioElementPlugin_artefacts/$BUILD_CONFIG/AAX/Eclipsa Audio Element Plugin.aaxplugin/Contents/MacOS/Eclipsa Audio Element Plugin"
AAX_RENDERER_PLUGIN_RESOURCES="../build/rendererplugin/RendererPlugin_artefacts/$BUILD_CONFIG/AAX/Eclipsa Audio Renderer.aaxplugin/Contents/Resources"
AAX_AUDIOELEMENT_PLUGIN_RESOURCES="../build/audioelementplugin/AudioElementPlugin_artefacts/$BUILD_CONFIG/AAX/Eclipsa Audio Element Plugin.aaxplugin/Contents/Resources"
AAX_RENDERER_PLUGIN="../build/rendererplugin/RendererPlugin_artefacts/$BUILD_CONFIG/AAX/Eclipsa Audio Renderer.aaxplugin"
AAX_AUDIOELEMENT_PLUGIN="../build/audioelementplugin/AudioElementPlugin_artefacts/$BUILD_CONFIG/AAX/Eclipsa Audio Element Plugin.aaxplugin"

# VST3 Plugin Paths
VST3_RENDERER_PLUGIN_BINARY="../build/rendererplugin/RendererPlugin_artefacts/$BUILD_CONFIG/VST3/Eclipsa Audio Renderer.vst3/Contents/MacOS/Eclipsa Audio Renderer"
VST3_AUDIOELEMENT_PLUGIN_BINARY="../build/audioelementplugin/AudioElementPlugin_artefacts/$BUILD_CONFIG/VST3/Eclipsa Audio Element Plugin.vst3/Contents/MacOS/Eclipsa Audio Element Plugin"
VST3_RENDERER_PLUGIN_RESOURCES="../build/rendererplugin/RendererPlugin_artefacts/$BUILD_CONFIG/VST3/Eclipsa Audio Renderer.vst3/Contents/Resources"
VST3_AUDIOELEMENT_PLUGIN_RESOURCES="../build/audioelementplugin/AudioElementPlugin_artefacts/$BUILD_CONFIG/VST3/Eclipsa Audio Element Plugin.vst3/Contents/Resources"
VST3_RENDERER_PLUGIN="../build/rendererplugin/RendererPlugin_artefacts/$BUILD_CONFIG/VST3/Eclipsa Audio Renderer.vst3"
VST3_AUDIOELEMENT_PLUGIN="../build/audioelementplugin/AudioElementPlugin_artefacts/$BUILD_CONFIG/VST3/Eclipsa Audio Element Plugin.vst3"

# AU Plugin Paths
AU_RENDERER_PLUGIN_BINARY="../build/rendererplugin/RendererPlugin_artefacts/$BUILD_CONFIG/AU/Eclipsa Audio Renderer.component/Contents/MacOS/Eclipsa Audio Renderer"
AU_AUDIOELEMENT_PLUGIN_BINARY="../build/audioelementplugin/AudioElementPlugin_artefacts/$BUILD_CONFIG/AU/Eclipsa Audio Element Plugin.component/Contents/MacOS/Eclipsa Audio Element Plugin"
AU_RENDERER_PLUGIN_RESOURCES="../build/rendererplugin/RendererPlugin_artefacts/$BUILD_CONFIG/AU/Eclipsa Audio Renderer.component/Contents/Resources"
AU_AUDIOELEMENT_PLUGIN_RESOURCES="../build/audioelementplugin/AudioElementPlugin_artefacts/$BUILD_CONFIG/AU/Eclipsa Audio Element Plugin.component/Contents/Resources"
AU_RENDERER_PLUGIN="../build/rendererplugin/RendererPlugin_artefacts/$BUILD_CONFIG/AU/Eclipsa Audio Renderer.component"
AU_AUDIOELEMENT_PLUGIN="../build/audioelementplugin/AudioElementPlugin_artefacts/$BUILD_CONFIG/AU/Eclipsa Audio Element Plugin.component"

# Define the directories to write the signed files to
# AAX Signing Directories
AAX_RENDERER_PLUGIN_SIGNING_DIRECTORY="../build/rendererplugin/RendererPlugin_artefacts/$BUILD_CONFIG/AAX/Signed"
AAX_AUDIOELEMENT_PLUGIN_SIGNING_DIRECTORY="../build/audioelementplugin/AudioElementPlugin_artefacts/$BUILD_CONFIG/AAX/Signed"
AAX_RENDERER_PLUGIN_SIGNED="$AAX_RENDERER_PLUGIN_SIGNING_DIRECTORY/Eclipsa Audio Renderer.aaxplugin"
AAX_AUDIOELEMENT_PLUGIN_SIGNED="$AAX_AUDIOELEMENT_PLUGIN_SIGNING_DIRECTORY/Eclipsa Audio Element Plugin.aaxplugin"

# VST3 Signing Directories
VST3_RENDERER_PLUGIN_SIGNING_DIRECTORY="../build/rendererplugin/RendererPlugin_artefacts/$BUILD_CONFIG/VST3/Signed"
VST3_AUDIOELEMENT_PLUGIN_SIGNING_DIRECTORY="../build/audioelementplugin/AudioElementPlugin_artefacts/$BUILD_CONFIG/VST3/Signed"
VST3_RENDERER_PLUGIN_SIGNED="$VST3_RENDERER_PLUGIN_SIGNING_DIRECTORY/Eclipsa Audio Renderer.vst3"
VST3_AUDIOELEMENT_PLUGIN_SIGNED="$VST3_AUDIOELEMENT_PLUGIN_SIGNING_DIRECTORY/Eclipsa Audio Element Plugin.vst3"

# AU Signing Directories
AU_RENDERER_PLUGIN_SIGNING_DIRECTORY="../build/rendererplugin/RendererPlugin_artefacts/$BUILD_CONFIG/AU/Signed"
AU_AUDIOELEMENT_PLUGIN_SIGNING_DIRECTORY="../build/audioelementplugin/AudioElementPlugin_artefacts/$BUILD_CONFIG/AU/Signed"
AU_RENDERER_PLUGIN_SIGNED="$AU_RENDERER_PLUGIN_SIGNING_DIRECTORY/Eclipsa Audio Renderer.component"
AU_AUDIOELEMENT_PLUGIN_SIGNED="$AU_AUDIOELEMENT_PLUGIN_SIGNING_DIRECTORY/Eclipsa Audio Element Plugin.component"

# Define the path to the signing tool
WRAP_TOOL_HELPER="./wraptool_helper.sh"

# Define output filenames
BRANCH_NAME=$(git rev-parse --abbrev-ref HEAD 2>/dev/null || echo "main")
if [ "$BRANCH_NAME" = "HEAD" ]; then
    # If we're in a detached HEAD state, or have checked out a tag, fetch the tag
    BRANCH_NAME=$(git describe --tags --exact-match 2>/dev/null)

    # If no tag is found, fallback to commit info
    if [ -z "$BRANCH_NAME" ]; then
        BRANCH_NAME=$(git rev-parse --short HEAD)
    fi
fi

INSTALLER_NAME="Eclipsa_${FORMAT_SUFFIX}_${BRANCH_NAME}.pkg"
FINAL_DMG_NAME="Eclipsa_Plugins_${FORMAT_SUFFIX}_${BRANCH_NAME}.dmg"
APP_NAME="Install Eclipsa Audio Plugins"
APP_DIR="./$APP_NAME.app"
TEMP_DMG_NAME="./temp_$FINAL_DMG_NAME"
PKG_NAME="EclipsaPlugins.pkg"

# Define work directories
DMG_STAGING_DIR="./dmg_staging"
TMP_ICONSET="./tmp.iconset"
RESIZED_BACKGROUND="./dmg_background.png"
MOUNT_POINT="/Volumes/$DMG_TITLE"

# ======================================================
# UTILITY FUNCTIONS
# ======================================================

# Error handling function
handle_error() {
    log "ERROR: $1 failed with exit code $2"
    exit $2
}

# Run a command with error handling
run_cmd() {
    local cmd="$1"
    local allow_fail="${2:-false}"
    
    log "> $cmd"
    if eval "$cmd"; then
        return 0
    elif [ "$allow_fail" = "true" ]; then
        log "WARNING: Command failed but continuing: $cmd"
        return 0
    else
        handle_error "$cmd" $?
    fi
}

# Function to check if a tool is available
check_tool() {
    if ! command -v "$1" &> /dev/null; then
        log "WARNING: Required tool '$1' is not installed. Some functionality may not be available."
        return 1
    fi
    return 0
}

# Check if the necessary tools are installed
check_requirements() {
    log_section "Checking requirements"
    check_tool pkgbuild
    check_tool productbuild
    check_tool codesign
    check_tool hdiutil
    check_tool osascript
    check_tool sips
    check_tool iconutil
}

# Function to sign a dylib, either using developer ID or ad-hoc
sign_dylib() {
    local dylib="$1"
    local resources="$2"
    
    # Skip if file doesn't exist
    if [ ! -f "$resources/$dylib" ]; then
        log "WARNING: Dylib not found: $resources/$dylib"
        return 0
    fi
    
    # Use developer ID if available, otherwise ad-hoc signing
    if [ -n "${DEVELOPER_ID_APPLICATION:-}" ]; then
        run_cmd "codesign -s \"$DEVELOPER_ID_APPLICATION\" --timestamp --deep --force \"$resources/$dylib\""
    else
        run_cmd "sudo codesign --force --deep --sign - \"$resources/$dylib\""
    fi
}

# Function to dynamically find and remove any ZMQ RPATH
adjust_rpath_and_sign() {
    local target_file="$1"

    if [ ! -f "$target_file" ]; then
        log "ERROR: File $target_file does not exist. Skipping."
        return 1
    fi

    log "Processing file: $target_file"

    # Find and remove ZMQ RPATH if present
    local RPATH_TO_REMOVE=$(otool -l "$target_file" | grep "path .*zeromq-build/lib" | sed -n 's/.*path \([^ ]*\).*/\1/p')
    if [ -n "$RPATH_TO_REMOVE" ]; then
        run_cmd "sudo install_name_tool -delete_rpath \"$RPATH_TO_REMOVE\" \"$target_file\""
        log "Removed RPATH: $RPATH_TO_REMOVE from $target_file"
    else
        log "No ZMQ RPATH found in $target_file, skipping."
    fi
    
    return 0
}

# Function to sign a plugin bundle using Apple's codesign
sign_plugin_bundle() {
    local plugin_path="$1"
    local signed_plugin_path="$2"
    local signing_dir=$(dirname "$signed_plugin_path")
    
    if [ ! -d "$plugin_path" ]; then
        log "ERROR: Plugin not found: $plugin_path"
        return 1
    fi
    
    # Prepare plugin for signing
    run_cmd "mkdir -p \"$signing_dir\""
    run_cmd "sudo rm -rf \"$signed_plugin_path\"" 
    run_cmd "sudo cp -R \"$plugin_path\" \"$signed_plugin_path\""
    run_cmd "sudo find \"$signed_plugin_path\" -name .DS_Store -delete"
    run_cmd "sudo chmod -R a+rw \"$signed_plugin_path\""
    
    # Sign with Developer ID or ad-hoc
    if [ -n "${DEVELOPER_ID_APPLICATION:-}" ]; then
        log "Signing plugin with Developer ID: $plugin_path"
        
        # Create entitlements file for hardened runtime
        local entitlements_file=$(mktemp)
        cat > "$entitlements_file" << EOF
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>com.apple.security.cs.disable-library-validation</key><true/>
    <key>com.apple.security.automation.apple-events</key><true/>
    <key>com.apple.security.device.audio-input</key><true/>
    <key>com.apple.security.device.microphone</key><true/>
    <key>com.apple.security.cs.allow-unsigned-executable-memory</key><true/>
    <key>com.apple.security.cs.allow-jit</key><true/>
    <key>com.apple.security.files.user-selected.read-write</key><true/>
    <key>com.apple.security.files.downloads.read-write</key><true/>
    <key>com.apple.security.temporary-exception.files.home-relative-path.read-write</key>
    <array>
        <string>/Library/Application Support/</string>
        <string>/Library/Logs/</string>
        <string>/Documents/</string>
        <string>/Music/</string>
    </array>
    <key>com.apple.security.network.client</key><true/>
    <key>com.apple.security.network.server</key><true/>
    <key>com.apple.security.device.camera</key><true/>
    <key>com.apple.security.files.bookmarks.app-scope</key><true/>
    <key>com.apple.security.files.bookmarks.document-scope</key><true/>
    <key>com.apple.security.print</key><true/>
    <key>com.apple.security.temporary-exception.shared-preference.read-write</key>
    <array>
        <string>com.apple.CoreAudio</string>
        <string>com.apple.audio.units</string>
        <string>com.avid.ProTools</string>
        <string>com.steinberg.cubase</string>
        <string>com.apple.logic10</string>
        <string>com.presonus.studioone</string>
        <string>com.cockos.reaper</string>
        <string>com.ableton.live</string>
        <string>com.motu.digitalperformer</string>
        <string>com.hindenburg.hindenburgpro</string>
    </array>
    <key>com.apple.security.temporary-exception.audio-unit-host</key><true/>
    <key>com.apple.security.cs.allow-dyld-environment-variables</key><true/>
</dict>
</plist>
EOF
        
        # Sign components hierarchically
        # 1. Frameworks
        if [ -d "$signed_plugin_path/Contents/Frameworks" ]; then
            find "$signed_plugin_path/Contents/Frameworks" -type f -name "*.dylib" -o -path "*.framework/*" 2>/dev/null | while read -r framework; do
                codesign -s "$DEVELOPER_ID_APPLICATION" --timestamp --options runtime --force "$framework" || log "WARNING: Failed to sign $framework but continuing..."
            done
        fi
        
        # 2. Executables in Resources
        if [ -d "$signed_plugin_path/Contents/Resources" ]; then
            find "$signed_plugin_path/Contents/Resources" -type f -perm +111 2>/dev/null | while read -r executable; do
                codesign -s "$DEVELOPER_ID_APPLICATION" --timestamp --options runtime --force "$executable" || log "WARNING: Failed to sign $executable but continuing..."
            done
        fi

        # 3. Main binaries
        if [ -d "$signed_plugin_path/Contents/MacOS" ]; then
            find "$signed_plugin_path/Contents/MacOS" -type f 2>/dev/null | while read -r binary; do
                codesign -s "$DEVELOPER_ID_APPLICATION" --timestamp --options runtime --entitlements "$entitlements_file" --force "$binary" || log "WARNING: Failed to sign $binary but continuing..."
            done
        fi
        
        # 4. Main bundle
        run_cmd "codesign -s \"$DEVELOPER_ID_APPLICATION\" --timestamp --options runtime --entitlements \"$entitlements_file\" --deep --force \"$signed_plugin_path\""
        rm -f "$entitlements_file"
        
        # Verify signature
        codesign -vvv --deep "$signed_plugin_path" || log "WARNING: Signature verification reported issues but continuing..."
    else
        log "Signing plugin with ad-hoc signature: $plugin_path"
        run_cmd "sudo codesign --force --deep --options runtime --sign - \"$signed_plugin_path\""
    fi
    
    log "Plugin signed: $signed_plugin_path"
    return 0
}

# Function to notarize a file and staple the ticket
notarize_and_staple() {
    local file_path="$1"
    local file_type="$2"
    
    if [ -z "${NOTARIZATION_KEYCHAIN_PROFILE:-}" ]; then
        log "Skipping notarization for $file_type (no keychain profile defined)"
        return 0
    fi
    
    log "Notarizing $file_type: $file_path"
    
    # Submit for notarization and wait for completion
    local notarization_output
    notarization_output=$(xcrun notarytool submit "$file_path" --keychain-profile "$NOTARIZATION_KEYCHAIN_PROFILE" --wait 2>&1)
    local result=$?
    
    log "Notarization submission output:"
    echo "$notarization_output"
    
    # Extract the submission ID from the output
    local submission_id=$(echo "$notarization_output" | grep "id:" | head -1 | awk '{print $2}')
    
    if [ $result -ne 0 ] || [ -z "$submission_id" ]; then
        log "ERROR: Notarization submission failed"
        return 1
    fi
    
    # Check notarization status
    if echo "$notarization_output" | grep -q "status: Invalid"; then
        log "Notarization FAILED. Getting detailed logs..."
        xcrun notarytool log "$submission_id" --keychain-profile "$NOTARIZATION_KEYCHAIN_PROFILE"
        return 1
    elif ! echo "$notarization_output" | grep -q "status: Accepted"; then
        log "Notarization status unclear. Check manually with: xcrun notarytool info $submission_id"
        return 1
    fi
    
    log "Notarization SUCCESSFUL. Stapling ticket to $file_type"
    sleep 5  # Brief delay for ticket availability
    
    # Try stapling with multiple attempts if needed
    local max_attempts=3
    for attempt in $(seq 1 $max_attempts); do
        log "Stapling attempt $attempt of $max_attempts"
        if xcrun stapler staple "$file_path"; then
            log "Successfully stapled $file_type"
            return 0
        fi
        
        if [ $attempt -lt $max_attempts ]; then
            log "Stapling attempt $attempt failed. Waiting before retry..."
            sleep 10
        fi
    done
    
    log "WARNING: Failed to staple notarization ticket after $max_attempts attempts."
    log "The application is properly notarized but not stapled."
    return 1
}

# Function to clean up temporary files on exit
cleanup() {
    log "Cleaning up temporary files"
    # Unmount DMG if still mounted
    if [ -d "$MOUNT_POINT" ]; then
        hdiutil detach "$MOUNT_POINT" -force &>/dev/null || true
    fi
    
    # Remove temporary files
    rm -rf "$DMG_STAGING_DIR" &>/dev/null || true
    rm -f "$TEMP_DMG_NAME" &>/dev/null || true
    rm -f "$RESIZED_BACKGROUND" &>/dev/null || true
    rm -rf "$TMP_ICONSET" &>/dev/null || true
    rm -f "./custom_dmg_icon.icns" &>/dev/null || true
    rm -f "./set_dmg_appearance.applescript" &>/dev/null || true
}

# Function to copy dylibs from build directory to plugin resources
copy_dylibs_to_plugin_resources() {
    local plugin_type="$1"
    local plugin_paths=("${@:2}")
    
    # Set base directory to ensure consistent path resolution
    local build_dir="${PWD%/*}/build"
    
    # Define source and target paths for dylibs
    local dylib_sources=(
        "$build_dir/_deps/zeromq-build/lib/libzmq.5.2.6.dylib:Resources/"
        "$build_dir/third_party/iamftools/lib/libiamf_tools.dylib:Resources/third_party/iamftools/lib/"
        "$build_dir/third_party/obr/lib/obr.dylib:Resources/third_party/obr/lib/"
    )
    
    log "Copying dylibs to $plugin_type plugin resources"
    
    for plugin_path in "${plugin_paths[@]}"; do
        local resources_path="$plugin_path/Contents/Resources"
        
        # Create resources directory structure
        run_cmd "sudo mkdir -p \"$resources_path/third_party/iamftools/lib\""
        run_cmd "sudo mkdir -p \"$resources_path/third_party/obr/lib\""
        
        # Copy each dylib if it exists
        for dylib_entry in "${dylib_sources[@]}"; do
            IFS=':' read -r src_file dest_dir <<< "$dylib_entry"
            if [ -f "$src_file" ]; then
                run_cmd "sudo cp \"$src_file\" \"$plugin_path/Contents/$dest_dir\""
                local lib_name=$(basename "$src_file")
                log "Copied $lib_name"
                
                # Create symlinks for ZeroMQ if needed
                if [[ "$lib_name" == "libzmq.5.2.6.dylib" ]]; then
                    run_cmd "sudo ln -sf \"libzmq.5.2.6.dylib\" \"$resources_path/libzmq.5.dylib\""
                    run_cmd "sudo ln -sf \"libzmq.5.2.6.dylib\" \"$resources_path/libzmq.dylib\""
                fi
            else
                log "WARNING: Library not found at $src_file"
            fi
        done
    done
}

# Update plugin binary to reference bundled dylibs via RPATH
update_plugin_rpath() {
    local plugin_type="$1"
    local plugin_paths=("${@:2}")
    
    log "Updating RPATHs for $plugin_type plugins"
    
    for plugin_path in "${plugin_paths[@]}"; do
        if [[ "$plugin_type" == "VST3" ]]; then
            local binary_path="$plugin_path/Contents/MacOS/$(basename "${plugin_path%.vst3}")"
        elif [[ "$plugin_type" == "AU" ]]; then
            local binary_path="$plugin_path/Contents/MacOS/$(basename "${plugin_path%.component}")"
        fi
        
        if [ ! -f "$binary_path" ]; then
            log "ERROR: Plugin binary not found at $binary_path"
            continue
        fi
        
        run_cmd "sudo chmod +w \"$binary_path\""
        
        # Get existing RPATHs
        local existing_rpaths=$(otool -l "$binary_path" | grep -A2 LC_RPATH | grep "path " | sed 's/.*path //g')
        
        # Remove any existing ZeroMQ RPATHs
        for rpath in $existing_rpaths; do
            if [[ "$rpath" == *"zeromq"* ]]; then
                run_cmd "install_name_tool -delete_rpath \"$rpath\" \"$binary_path\""
                log "Removed RPATH: $rpath"
            fi
        done
        
        # Define RPATHs to add
        local rpaths=(
            "@loader_path/../Resources/"
            "@loader_path/../Resources/third_party/iamftools/lib/"
            "@loader_path/../Resources/third_party/obr/lib/"
        )
        
        # Add each RPATH if it doesn't already exist
        for rpath in "${rpaths[@]}"; do
            if ! echo "$existing_rpaths" | grep -q "$rpath"; then
                run_cmd "install_name_tool -add_rpath \"$rpath\" \"$binary_path\"" true
            else
                log "RPATH $rpath already exists, skipping"
            fi
        done
        
        log "Updated RPATHs for $(basename "$binary_path")"
    done
}

# Specialized function to create a .pkg file with custom icon
create_custom_pkg() {
    local source_pkg="$1"
    local final_pkg="$2"
    local icon_file="$3"
    
    log "Creating pkg with custom icon"
    
    # Create iconset directory
    local tmp_iconset="./pkg_icons.iconset"
    run_cmd "mkdir -p \"$tmp_iconset\""
    
    # Generate icons at different sizes
    for size in 16 32 128 256 512; do
        run_cmd "sips -z $size $size \"$icon_file\" --out \"$tmp_iconset/icon_${size}x${size}.png\" > /dev/null 2>&1"
        doubleSize=$((size * 2))
        run_cmd "sips -z $doubleSize $doubleSize \"$icon_file\" --out \"$tmp_iconset/icon_${size}x${size}@2x.png\" > /dev/null 2>&1"
    done
    
    # Convert iconset to icns
    local tmp_icns="./pkg_icon.icns"
    run_cmd "iconutil -c icns \"$tmp_iconset\" -o \"$tmp_icns\""
    
    # Create a temporary directory for package customization
    local pkg_work_dir="./tmp_pkg_work"
    run_cmd "mkdir -p \"$pkg_work_dir\""
    
    # Extract the original package
    log "Extracting original package"
    run_cmd "pkgutil --expand \"$source_pkg\" \"$pkg_work_dir/expanded\""
    
    # Add resources directory if it doesn't exist
    run_cmd "mkdir -p \"$pkg_work_dir/expanded/Resources\""
    
    # Copy the icon file as 'background' and 'package.icns' for compatibility
    run_cmd "cp \"$tmp_icns\" \"$pkg_work_dir/expanded/Resources/background\""
    run_cmd "cp \"$tmp_icns\" \"$pkg_work_dir/expanded/Resources/package.icns\""
    
    # Add icon references to Distribution file if it exists
    if [ -f "$pkg_work_dir/expanded/Distribution" ]; then
        if ! grep -q "background-image" "$pkg_work_dir/expanded/Distribution"; then
            log "Adding icon references to Distribution file"
            sed -i '' 's|</installer-gui-script>|    <background file="background" alignment="topleft" scaling="none"/>\n    <background-darkAqua file="background" alignment="topleft" scaling="none"/>\n</installer-gui-script>|g' "$pkg_work_dir/expanded/Distribution"
        fi
    fi
    
    # Flatten the package back
    log "Creating final custom icon package"
    run_cmd "pkgutil --flatten \"$pkg_work_dir/expanded\" \"$final_pkg\""
    
    # Directly sign the resulting package if we have a Developer ID Installer
    if [ -n "${DEVELOPER_ID_INSTALLER:-}" ]; then
        log "Signing the custom icon package"
        run_cmd "productsign --sign \"$DEVELOPER_ID_INSTALLER\" \"$final_pkg\" \"${final_pkg}.signed\""
        run_cmd "mv \"${final_pkg}.signed\" \"$final_pkg\""
        
        # Verify the signature
        log "Verifying package signature"
        run_cmd "pkgutil --check-signature \"$final_pkg\"" || log "WARNING: Custom package signature verification failed"
    fi
    
    # Apply resource fork icon method
    log "Applying resource fork icon method"
    local temp_rsrc_def="$pkg_work_dir/pkg_icon.rsrc"
    echo "read 'icns' (-16455) \"$tmp_icns\";" > "$temp_rsrc_def"
    
    # Use Rez and SetFile to apply the icon directly
    if xcrun -f Rez &> /dev/null && xcrun -f SetFile &> /dev/null; then
        xcrun Rez "$temp_rsrc_def" -o "$final_pkg" -append || log "WARNING: Rez command failed but continuing..."
        xcrun SetFile -a C "$final_pkg" || log "WARNING: SetFile command failed but continuing..."
    fi
    
    # Clean up
    run_cmd "rm -rf \"$pkg_work_dir\""
    run_cmd "rm -rf \"$tmp_iconset\""
    run_cmd "rm -f \"$tmp_icns\""
    
    log "Custom icon package created at $final_pkg"
}

# ======================================================
# MAIN PACKAGING PROCESS
# ======================================================

# Set up cleanup trap
trap cleanup EXIT

# Check requirements
check_requirements

log_section "Starting Eclipsa Audio Plugins packaging for format: $PLUGIN_FORMAT"
log "Version: $VERSION"
log "Branch: $BRANCH_NAME"

# Clean up any existing package files
run_cmd "rm -f ./*.pkg"

# ======================================================
# STEP 1: PLUGIN PREPARATION AND CODE SIGNING
# ======================================================
    
# Sign all dylibs in a plugin
sign_plugin_dylibs() {
    local resources_path="$1"
    
    # Sign all ZeroMQ and dependency libraries
    local dylibs=("libzmq.5.2.6.dylib" "libzmq.5.dylib" "libzmq.dylib" 
                 "third_party/iamftools/lib/libiamf_tools.dylib" 
                 "third_party/obr/lib/obr.dylib")
    
    for dylib in "${dylibs[@]}"; do
        sign_dylib "$dylib" "$resources_path"
    done
}

process_aax_plugins() {
    log_section "AAX plugin preparation and code signing"

    # Adjust RPATH for each plugin
    adjust_rpath_and_sign "$AAX_RENDERER_PLUGIN_BINARY"
    adjust_rpath_and_sign "$AAX_AUDIOELEMENT_PLUGIN_BINARY"

    # Sign all dylibs
    sign_plugin_dylibs "$AAX_RENDERER_PLUGIN_RESOURCES"
    sign_plugin_dylibs "$AAX_AUDIOELEMENT_PLUGIN_RESOURCES"

    # Create directories for signed plugins
    run_cmd "mkdir -p \"$AAX_RENDERER_PLUGIN_SIGNING_DIRECTORY\""
    run_cmd "mkdir -p \"$AAX_AUDIOELEMENT_PLUGIN_SIGNING_DIRECTORY\""

    # Sign the AAX plugins
    if [ -e "$WRAP_TOOL_HELPER" ]; then
        log "Using wraptool for AAX plugin signing"
        run_cmd "$WRAP_TOOL_HELPER \"$AAX_RENDERER_PLUGIN\" \"$AAX_RENDERER_PLUGIN_SIGNED\" \"$AAX_AUDIOELEMENT_PLUGIN\" \"$AAX_AUDIOELEMENT_PLUGIN_SIGNED\""
    else
        log "Applying ad-hoc code signing for AAX plugins"
        run_cmd "sudo codesign --force --deep --sign - \"$AAX_RENDERER_PLUGIN_BINARY\""
        run_cmd "sudo codesign --force --deep --sign - \"$AAX_AUDIOELEMENT_PLUGIN_BINARY\""
        
        # Copy plugins to signing directory
        run_cmd "sudo cp -R \"$AAX_RENDERER_PLUGIN\" \"$AAX_RENDERER_PLUGIN_SIGNED\""
        run_cmd "sudo cp -R \"$AAX_AUDIOELEMENT_PLUGIN\" \"$AAX_AUDIOELEMENT_PLUGIN_SIGNED\""
    fi
}

process_vst3_plugins() {
    log_section "VST3 plugin preparation and code signing"

    # Adjust RPATH for each plugin
    adjust_rpath_and_sign "$VST3_RENDERER_PLUGIN_BINARY"
    adjust_rpath_and_sign "$VST3_AUDIOELEMENT_PLUGIN_BINARY"

    # Copy dylibs to plugin resources before signing
    copy_dylibs_to_plugin_resources "VST3" "$VST3_RENDERER_PLUGIN" "$VST3_AUDIOELEMENT_PLUGIN"

    # Update RPATHs in plugin binaries to point to the bundled dylibs
    update_plugin_rpath "VST3" "$VST3_RENDERER_PLUGIN" "$VST3_AUDIOELEMENT_PLUGIN"

    # Sign all dylibs
    sign_plugin_dylibs "$VST3_RENDERER_PLUGIN_RESOURCES"
    sign_plugin_dylibs "$VST3_AUDIOELEMENT_PLUGIN_RESOURCES"

    # Sign VST3 plugins using Apple codesign
    log "Signing VST3 plugins using Apple codesign"
    sign_plugin_bundle "$VST3_RENDERER_PLUGIN" "$VST3_RENDERER_PLUGIN_SIGNED"
    sign_plugin_bundle "$VST3_AUDIOELEMENT_PLUGIN" "$VST3_AUDIOELEMENT_PLUGIN_SIGNED"
}

process_au_plugins() {
    log_section "AU plugin preparation and code signing"

    # Adjust RPATH for each plugin
    adjust_rpath_and_sign "$AU_RENDERER_PLUGIN_BINARY"
    adjust_rpath_and_sign "$AU_AUDIOELEMENT_PLUGIN_BINARY"

    # Copy dylibs to plugin resources before signing
    copy_dylibs_to_plugin_resources "AU" "$AU_RENDERER_PLUGIN" "$AU_AUDIOELEMENT_PLUGIN"

    # Update RPATHs in plugin binaries to point to the bundled dylibs
    update_plugin_rpath "AU" "$AU_RENDERER_PLUGIN" "$AU_AUDIOELEMENT_PLUGIN"

    # Sign all dylibs
    sign_plugin_dylibs "$AU_RENDERER_PLUGIN_RESOURCES"
    sign_plugin_dylibs "$AU_AUDIOELEMENT_PLUGIN_RESOURCES"

    # Sign AU plugins using Apple codesign
    log "Signing AU plugins using Apple codesign"
    sign_plugin_bundle "$AU_RENDERER_PLUGIN" "$AU_RENDERER_PLUGIN_SIGNED"
    sign_plugin_bundle "$AU_AUDIOELEMENT_PLUGIN" "$AU_AUDIOELEMENT_PLUGIN_SIGNED"
}

# Process plugins based on selected format
case "$PLUGIN_FORMAT" in
    aax)
        process_aax_plugins
        ;;
esac

case "$PLUGIN_FORMAT" in
    vst3)
        process_vst3_plugins
        ;;
esac

case "$PLUGIN_FORMAT" in
    au)
        process_au_plugins
        ;;
esac

# ======================================================
# STEP 2: PKG INSTALLER CREATION
# ======================================================
log_section "Creating PKG installer"

# Process plugins based on selected format
case "$PLUGIN_FORMAT" in
    aax)
        # AAX: Single package for system installation only
        log "Preparing directory structure for AAX packaging"
        run_cmd "sudo rm -rf ./plugins_pkg"
        run_cmd "mkdir -p \"./plugins_pkg/Library/Application Support/Eclipsa Audio Plugins\""
        run_cmd "mkdir -p \"./plugins_pkg/Library/Application Support/Avid/Audio/Plug-Ins\""
        
        # Copy AAX plugins
        run_cmd "sudo cp -R \"$AAX_RENDERER_PLUGIN_SIGNED\" \"$AAX_AUDIOELEMENT_PLUGIN_SIGNED\" \
            \"./plugins_pkg/Library/Application Support/Avid/Audio/Plug-Ins\""
        
        # Copy license for AAX
        run_cmd "sudo cp \"$LICENSE_FILE\" \"./plugins_pkg/Library/Application Support/Eclipsa Audio Plugins/\""
        
        # Set ownership and permissions for AAX
        CURRENT_USER=$(whoami)
        CURRENT_GROUP=$(id -g -n "$(whoami)")
        run_cmd "sudo chown -R $CURRENT_USER:$CURRENT_GROUP ./plugins_pkg"
        run_cmd "sudo chmod -R 755 \"./plugins_pkg\""
        
        # Build single AAX package
        run_cmd "pkgbuild --root \"./plugins_pkg\" --identifier \"$BUNDLE_IDENTIFIER\" --install-location \"/\" --version \"$VERSION\" \"./$PKG_NAME\""
        ;;
        
    vst3)
        # VST3: Create relocatable package with postinstall script to handle location
        log "Preparing directory structure for VST3 packaging (relocatable)"
        
        # Clean up any existing package directories
        run_cmd "sudo rm -rf ./plugins_pkg"
        
        # Create package structure - ONLY use staging area to avoid file conflicts
        # The postinstall script will handle moving to the correct final location
        run_cmd "mkdir -p \"./plugins_pkg/tmp/EclipsaVST3\""
        run_cmd "mkdir -p \"./plugins_pkg/Library/Application Support/Eclipsa Audio Plugins\""
        
        # Handle VST3 plugins, fixing nested bundles if needed
        if [ -d "$VST3_RENDERER_PLUGIN_SIGNED/Eclipsa Audio Renderer.vst3" ]; then
            log "Warning: Found nested VST3 bundle structure, fixing..."
            run_cmd "sudo mkdir -p \"./vst3_fixed\""
            run_cmd "sudo cp -R \"$VST3_RENDERER_PLUGIN_SIGNED/Eclipsa Audio Renderer.vst3\" \
                \"$VST3_AUDIOELEMENT_PLUGIN_SIGNED/Eclipsa Audio Element Plugin.vst3\" \"./vst3_fixed/\""
            # Copy ONLY to staging area - postinstall will handle final placement
            run_cmd "sudo cp -R \"./vst3_fixed/\"* \"./plugins_pkg/tmp/EclipsaVST3/\""
            run_cmd "sudo rm -rf \"./vst3_fixed\""
        else
            # Copy ONLY to staging area - postinstall will handle final placement
            run_cmd "sudo cp -R \"$VST3_RENDERER_PLUGIN_SIGNED\" \"$VST3_AUDIOELEMENT_PLUGIN_SIGNED\" \
                \"./plugins_pkg/tmp/EclipsaVST3/\""
        fi
        
        # Copy license
        run_cmd "sudo cp \"$LICENSE_FILE\" \"./plugins_pkg/Library/Application Support/Eclipsa Audio Plugins/\""
        
        # Set ownership and permissions
        CURRENT_USER=$(whoami)
        CURRENT_GROUP=$(id -g -n "$(whoami)")
        run_cmd "sudo chown -R $CURRENT_USER:$CURRENT_GROUP ./plugins_pkg"
        run_cmd "sudo chmod -R 755 \"./plugins_pkg\""
        
        # Create a temporary scripts directory with both preinstall and postinstall scripts for VST3
        run_cmd "mkdir -p ./vst3_scripts"
        run_cmd "cp ./preinstall_vst3 ./vst3_scripts/preinstall"
        run_cmd "cp ./postinstall_vst3 ./vst3_scripts/postinstall"
        run_cmd "chmod +x ./vst3_scripts/preinstall"
        run_cmd "chmod +x ./vst3_scripts/postinstall"
        
        # Build package with VST3-specific preinstall and postinstall scripts
        run_cmd "pkgbuild --root \"./plugins_pkg\" --identifier \"$BUNDLE_IDENTIFIER\" --install-location \"/\" --scripts \"./vst3_scripts\" --version \"$VERSION\" \"./$PKG_NAME\""
        
        # Clean up temporary scripts directory
        run_cmd "rm -rf ./vst3_scripts"
        ;;
    au)
     # AU: Create relocatable package with postinstall script to handle location
        log "Preparing directory structure for AU packaging (relocatable)"

        # Clean up any existing package directories
        run_cmd "sudo rm -rf ./plugins_pkg"
        
        # Create package structure - ONLY use staging area to avoid file conflicts
        # The postinstall script will handle moving to the correct final location
        run_cmd "mkdir -p \"./plugins_pkg/tmp/EclipsaAU\""
        run_cmd "mkdir -p \"./plugins_pkg/Library/Application Support/Eclipsa Audio Plugins\""

        # Handle AU plugins, fixing nested bundles if needed
        if [ -d "$AU_RENDERER_PLUGIN_SIGNED/Eclipsa Audio Renderer.component" ]; then
            log "Warning: Found nested AU bundle structure, fixing..."
            run_cmd "sudo mkdir -p \"./au_fixed\""
            run_cmd "sudo cp -R \"$AU_RENDERER_PLUGIN_SIGNED/Eclipsa Audio Renderer.component\" \
                \"$AU_AUDIOELEMENT_PLUGIN_SIGNED/Eclipsa Audio Element Plugin.component\" \"./au_fixed/\""
            # Copy ONLY to staging area - postinstall will handle final placement
            run_cmd "sudo cp -R \"./au_fixed/\"* \"./plugins_pkg/tmp/EclipsaAU/\""
            run_cmd "sudo rm -rf \"./au_fixed\""
        else
            # Copy ONLY to staging area - postinstall will handle final placement
            run_cmd "sudo cp -R \"$AU_RENDERER_PLUGIN_SIGNED\" \"$AU_AUDIOELEMENT_PLUGIN_SIGNED\" \
                \"./plugins_pkg/tmp/EclipsaAU/\""
        fi
        
        # Copy license
        run_cmd "sudo cp \"$LICENSE_FILE\" \"./plugins_pkg/Library/Application Support/Eclipsa Audio Plugins/\""
        
        # Set ownership and permissions
        CURRENT_USER=$(whoami)
        CURRENT_GROUP=$(id -g -n "$(whoami)")
        run_cmd "sudo chown -R $CURRENT_USER:$CURRENT_GROUP ./plugins_pkg"
        run_cmd "sudo chmod -R 755 \"./plugins_pkg\""

        # Create a temporary scripts directory with both preinstall and postinstall scripts for AU
        run_cmd "mkdir -p ./au_scripts"
        run_cmd "cp ./preinstall_au ./au_scripts/preinstall"
        run_cmd "cp ./postinstall_au ./au_scripts/postinstall"
        run_cmd "chmod +x ./au_scripts/preinstall"
        run_cmd "chmod +x ./au_scripts/postinstall"

        # Build package with AU-specific preinstall and postinstall scripts
        run_cmd "pkgbuild --root \"./plugins_pkg\" --identifier \"$BUNDLE_IDENTIFIER\" --install-location \"/\" --scripts \"./au_scripts\" --version \"$VERSION\" \"./$PKG_NAME\""

        # Clean up temporary scripts directory
        run_cmd "rm -rf ./au_scripts"
        ;;
esac

# Build the distribution, signing it if developer ID available
if [ -n "${DEVELOPER_ID_INSTALLER:-}" ]; then
    log "Building signed distribution package"
    run_cmd "productbuild --distribution \"$DISTRIBUTION_XML\" --resources \"./\" --package-path \"./\" \"./temp_$INSTALLER_NAME\" --sign \"$DEVELOPER_ID_INSTALLER\""
else
    log "Building unsigned distribution package"
    run_cmd "productbuild --distribution \"$DISTRIBUTION_XML\" --resources \"./\" --package-path \"./\" \"./temp_$INSTALLER_NAME\""
fi

# Create the custom icon version of the package
create_custom_pkg "./temp_$INSTALLER_NAME" "./$INSTALLER_NAME" "$CUSTOM_ICON_FILE"

# Remove the temporary package
run_cmd "rm -f \"./temp_$INSTALLER_NAME\""

log "PKG installer created at ./$INSTALLER_NAME"

# Option to notarize the PKG installer if credentials are configured
if [ -n "${NOTARIZATION_KEYCHAIN_PROFILE:-}" ]; then
    notarize_and_staple "./$INSTALLER_NAME" "PKG installer"
fi

# ======================================================
# STEP 3: DMG CREATION
# ======================================================
log_section "Creating DMG installer"

# Clean up previous staging directory and DMG
run_cmd "rm -rf \"$DMG_STAGING_DIR\""
run_cmd "rm -f \"./$FINAL_DMG_NAME\""
run_cmd "rm -f \"$TEMP_DMG_NAME\""
run_cmd "mkdir -p \"$DMG_STAGING_DIR\""

# Create a resized background image
if [ -f "$DMG_BACKGROUND_FILE" ]; then
    log "Creating resized background image"
    run_cmd "sips -Z 640 \"$DMG_BACKGROUND_FILE\" --out \"$RESIZED_BACKGROUND\"" || \
    run_cmd "cp \"$DMG_BACKGROUND_FILE\" \"$RESIZED_BACKGROUND\""
else
    log "WARNING: Background image not found at $DMG_BACKGROUND_FILE"
fi

# Copy the PKG installer to the DMG staging directory
log "Adding PKG installer to DMG"
run_cmd "cp \"./$INSTALLER_NAME\" \"$DMG_STAGING_DIR/\""

# Create Documentation and Licenses folders
log "Creating Documentation and Licenses folders"
run_cmd "mkdir -p \"$DMG_STAGING_DIR/Documentation\""
run_cmd "mkdir -p \"$DMG_STAGING_DIR/Licenses\""

# Copy LICENSE file to Licenses folder
if [ -f "$LICENSE_FILE" ]; then
    run_cmd "cp \"$LICENSE_FILE\" \"$DMG_STAGING_DIR/Licenses/LICENSE.txt\""
fi

# Copy THIRD_PARTY_LICENSES.txt if it exists
if [ -f "../third_party/THIRD_PARTY_LICENSES.txt" ]; then
    run_cmd "cp \"../third_party/THIRD_PARTY_LICENSES.txt\" \"$DMG_STAGING_DIR/Licenses/\""
fi

# Create a comprehensive documentation file with format-specific information
generate_documentation() {
    local output_file="$DMG_STAGING_DIR/Documentation/Documentation.txt"
    
    # Common documentation header
    cat > "$output_file" << EOF
# Eclipsa Audio Plugins

The Eclipsa Audio Renderer plugin provides music and audio creators with an intuitive, 
user-friendly interface for authoring IAMF audio files. Designed to integrate seamlessly 
into existing workflows as a plugin for Avid Pro Tools, it allows professionals to work 
easily with immersive audio formats.

## Key Features

• 3D Audio Capture: Position tracks in 3D space
• Audio Structure Definition: Define audio structure according to IAMF standard
• Real-time Monitoring: Hear your mix as intended during creation
• Audio Transcoding: Convert audio streams to target formats
• IAMF File Encoding: Create fully IAMF-compliant files
• Audio and Video Muxing: Combine 3D audio with video files

## Plugin Components

The plugin consists of two components:

1. Eclipsa Audio Element Plugin: 
   Position and manipulate audio objects and channels in 3D space.

2. Eclipsa Audio Renderer Plugin: 
   Monitor, configure, and encode the final audio mix into the IAMF format.

## Supported Formats
EOF

    # Format-specific installation information
    case "$PLUGIN_FORMAT" in
        aax)
            cat >> "$output_file" << EOF
This package contains AAX plugins compatible with Pro Tools.

## Installation Locations
• AAX plugins: /Library/Application Support/Avid/Audio/Plug-Ins
EOF
            ;;
        vst3)
            cat >> "$output_file" << EOF
This package contains VST3 plugins compatible with VST3-supporting DAWs.

## Installation Locations
• VST3 plugins: /Library/Audio/Plug-Ins/VST3
EOF
            ;;
        au)
            cat >> "$output_file" << EOF
This package contains AU plugins compatible with AU-supporting DAWs.

## Installation Locations
• AAX plugins: /Library/Application Support/Avid/Audio/Plug-Ins
• VST3 plugins: /Library/Audio/Plug-Ins/VST3
• AU plugins: /Library/Audio/Plug-Ins/Components
EOF
            ;;
    esac

    # Common documentation footer
    cat >> "$output_file" << EOF

## Basic Setup

1. Add the Eclipsa Audio Renderer Plugin to your Mix Bus (5th Order Ambisonics)
2. Create an audio element in the Renderer Plugin
3. Add an Audio Element Plugin to each track you want to spatialize
4. Route each track to the appropriate audio element

Note: For proper setup, instantiate the Eclipsa Audio Renderer Plugin first and create 
an audio element, as the Eclipsa Audio Element Plugin requires this configuration.

## For Detailed Documentation

For complete step-by-step instructions, workflow guides, and advanced features, 
please visit: $DOCS_URL

© $COPYRIGHT_YEAR $COMPANY_NAME. All rights reserved.
EOF
}

# Generate documentation
generate_documentation

# Create hidden folder for DMG appearance files
run_cmd "mkdir -p \"$DMG_STAGING_DIR/.background\""

if [ -f "$RESIZED_BACKGROUND" ]; then
    run_cmd "cp \"$RESIZED_BACKGROUND\" \"$DMG_STAGING_DIR/.background/background.png\""
fi    # Create a temporary DMG
    log "Creating temporary DMG for format $PLUGIN_FORMAT..."
    run_cmd "hdiutil create -volname \"$DMG_TITLE\" -srcfolder \"$DMG_STAGING_DIR\" -ov -format UDRW \"$TEMP_DMG_NAME\""

# Mount the DMG
log "Mounting DMG to customize appearance..."

# Unmount first in case it's already mounted
if [ -d "$MOUNT_POINT" ]; then
    run_cmd "hdiutil detach \"$MOUNT_POINT\" -force" true
fi

run_cmd "hdiutil attach -readwrite -noverify -noautoopen \"$TEMP_DMG_NAME\"" true

# Wait a moment for the mount to complete
sleep 3

if [ -d "$MOUNT_POINT" ]; then
    log "DMG mounted successfully at $MOUNT_POINT"
    
    # Set the custom icon for the volume
    if [ -f "$CUSTOM_ICON_FILE" ]; then
        log "Applying custom icon to DMG volume..."
        
        # Create temporary directory for conversion
        run_cmd "mkdir -p \"$TMP_ICONSET\""
        
        # Create various sized icons from the source PNG
        run_cmd "sips -z 512 512 \"$CUSTOM_ICON_FILE\" --out \"$TMP_ICONSET/icon_512x512.png\" > /dev/null 2>&1"
        run_cmd "cp \"$CUSTOM_ICON_FILE\" \"$TMP_ICONSET/icon_512x512@2x.png\" > /dev/null 2>&1"
        
        # Create icns file
        run_cmd "iconutil -c icns \"$TMP_ICONSET\" -o \"./custom_dmg_icon.icns\""
        
        if [ -f "./custom_dmg_icon.icns" ]; then
            # Copy the icon to the volume
            run_cmd "cp \"./custom_dmg_icon.icns\" \"$MOUNT_POINT/.VolumeIcon.icns\""
            
            # Set the custom icon flag
            run_cmd "xattr -wx com.apple.FinderInfo \"0000000000000000040000000000000000000000000000000000000000000000\" \"$MOUNT_POINT\""
            
            log "Custom volume icon applied"
        else
            log "WARNING: Failed to create .icns file from PNG"
        fi
        
        # Clean up temporary iconset
        run_cmd "rm -rf \"$TMP_ICONSET\""
    fi
    
    # Create a custom view settings file
    log "Setting DMG appearance..."
    
    # Create AppleScript to set DMG appearance with improved layout
    cat > "./set_dmg_appearance.applescript" << EOF
tell application "Finder"
    tell disk "$DMG_TITLE"
        open
        
        -- Wait a moment for window to be ready
        delay 1
        
        -- Basic window setup
        set current view of container window to icon view
        set toolbar visible of container window to false
        set statusbar visible of container window to false
        
        -- Set window dimensions
        set the bounds of container window to $DMG_WINDOW_SIZE
        
        -- Configure the icon view options
        set theViewOptions to the icon view options of container window
        set arrangement of theViewOptions to not arranged
        set icon size of theViewOptions to 80
        set text size of theViewOptions to 11
        set label position of theViewOptions to bottom
        set background picture of theViewOptions to file ".background:background.png"
        
        -- Position installer pkg and folders
        set position of item "$INSTALLER_NAME" of container window to {350, 220}
        set position of item "Documentation" of container window to {250, 370}
        set position of item "Licenses" of container window to {450, 370}
        
        -- Skip trying to hide .background folder as this causes errors
        -- Only attempt to hide .DS_Store if it exists
        if exists item ".DS_Store" of container window then
            try
                set visible of item ".DS_Store" of container window to false
            end try
        end if
        
        update without registering applications
        delay 1
        
        -- Close and reopen to ensure settings are applied
        close
        open
        
        -- Give Finder a moment to register the changes
        delay 2
    end tell
end tell
EOF
    
    # Run the AppleScript to set DMG appearance
    log "Executing AppleScript to configure DMG appearance"
    osascript "./set_dmg_appearance.applescript" || log "WARNING: AppleScript failed but continuing..."
    rm -f "./set_dmg_appearance.applescript"
    
    # Wait a moment before detaching
    sleep 2
    
    # Detach the DMG
    log "Detaching DMG..."
    run_cmd "hdiutil detach \"$MOUNT_POINT\" -force"
else
    log "WARNING: Failed to mount DMG at $MOUNT_POINT - appearance customization skipped"
fi

# Convert the DMG to compressed format
log "Converting DMG to final compressed format..."
run_cmd "hdiutil convert \"$TEMP_DMG_NAME\" -format UDZO -imagekey zlib-level=9 -o \"./$FINAL_DMG_NAME\""

# Clean up
run_cmd "rm -f \"$TEMP_DMG_NAME\""
run_cmd "rm -rf \"$DMG_STAGING_DIR\""

if [ -f "./$FINAL_DMG_NAME" ]; then
    log "DMG successfully created at ./$FINAL_DMG_NAME"
    
    # Option to notarize the DMG if credentials are configured
    if [ -n "${NOTARIZATION_KEYCHAIN_PROFILE:-}" ]; then
        notarize_and_staple "./$FINAL_DMG_NAME" "DMG file"
    fi
else
    log "ERROR: DMG creation failed."
    exit 1
fi

# Clean up staging files
run_cmd "sudo rm -rf \"./plugins_pkg\""

log_section "Packaging process complete"
log "Installer package: ./$INSTALLER_NAME"
log "DMG installer: ./$FINAL_DMG_NAME"

exit 0
