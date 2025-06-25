#!/bin/bash -eu
#
# CI-specific packaging script for Eclipsa Plugins
#
# This script is intended to be run within a GitHub Actions macOS environment.
# It performs the following steps:
# 1. Adjusts RPATHs in plugin binaries.
# 2. Signs dylibs and plugin binaries using Apple Developer ID Application certificate.
# 3. Copies signed plugins to the output directory.
#
# It expects the following environment variables to be set by the CI workflow:
# - APPLE_SIGNING_IDENTITY: The "Developer ID Application: ..." string.
# - KEYCHAIN_PATH: Path to the temporary keychain containing the certs.
# - KEYCHAIN_PASSWORD: Password for the temporary keychain.
# - BUILD_CONFIG: Build configuration (e.g., Release, RelWithDebInfo). Defaults to Release.

# --- Configuration ---
BUILD_CONFIG="${BUILD_CONFIG:-Release}"

# Sanitize branch name to avoid invalid characters in artifact/installer names
BRANCH_NAME_RAW="${BRANCH_NAME:-unknown_branch}"
BRANCH_NAME="${BRANCH_NAME_RAW//\//_}"

SKIP_PACE_WRAPPING="${SKIP_PACE_WRAPPING:-false}"

# Plugin format selection (AAX, VST3 or AU only)
PLUGIN_FORMAT="${PLUGIN_FORMAT:-aax}" # Default to AAX, options: aax, vst3, au

# Set format suffix based on plugin format
case "$PLUGIN_FORMAT" in
    aax)
        FORMAT_SUFFIX="AAX"
        ;;
    vst3)
        FORMAT_SUFFIX="VST3"
        ;;
    au)
        FORMAT_SUFFIX="AU"
        ;;
    *)
        echo "Error: Invalid plugin format. Use PLUGIN_FORMAT=aax , PLUGIN_FORMAT=vst3 or PLUGIN_FORMAT=au."
        exit 1
        ;;
esac

# Apple Credentials & Identities
DEV_APP_SIGNING_IDENTITY="${APPLE_SIGNING_IDENTITY:?Error: APPLE_SIGNING_IDENTITY env var not set.}"
DEV_INSTALLER_IDENTITY="${APPLE_INSTALLER_IDENTITY:?Error: APPLE_INSTALLER_IDENTITY env var not set.}"
APPLE_TEAM_ID="${APPLE_TEAM_ID:?Error: APPLE_TEAM_ID env var not set.}"
APPLE_ACCOUNT_EMAIL="${APPLE_ACCOUNT_EMAIL:?Error: APPLE_ACCOUNT_EMAIL env var not set.}"
APPLE_APP_SPECIFIC_PASSWORD="${APPLE_APP_SPECIFIC_PASSWORD:?Error: APPLE_APP_SPECIFIC_PASSWORD env var not set.}"
KEYCHAIN_PATH="${KEYCHAIN_PATH:?Error: KEYCHAIN_PATH env var not set.}"
KEYCHAIN_PASSWORD="${KEYCHAIN_PASSWORD}"

# Extract the signing identities - handle different formats
# The input might be in the format:   1) 41BB6575983E1E97CC536403262FC40B38CDBC54 "Developer ID Application: A-CX, LLC (***)"
# For regular codesigning, we use the fingerprint (hash) directly
# For PACE wraptool, we need the full certificate name

# Store the original full identity for PACE wraptool
DEV_APP_SIGNING_IDENTITY_FULL="$DEV_APP_SIGNING_IDENTITY"

if [[ "$DEV_APP_SIGNING_IDENTITY" == *")"* ]]; then
    # Extract the full certificate name for PACE wraptool (between quotes)
    # Input format: 1) 41BB6575983E1E97CC536403262FC40B38CDBC54 "Developer ID Application: A-CX, LLC (***)"
    DEV_APP_SIGNING_IDENTITY_FULL=$(echo "$DEV_APP_SIGNING_IDENTITY" | sed -E 's/.*[0-9A-F]{40} "(.*)"/\1/')
    echo "Full signing identity for PACE: $DEV_APP_SIGNING_IDENTITY_FULL"
    
    # Extract the fingerprint/hash part (the 40-character hex string) for regular codesigning
    DEV_APP_SIGNING_IDENTITY=$(echo "$DEV_APP_SIGNING_IDENTITY" | grep -o -E '[0-9A-F]{40}')
    echo "Extracted signing identity hash: $DEV_APP_SIGNING_IDENTITY"
fi

if [[ "$DEV_INSTALLER_IDENTITY" == *")"* ]]; then
    # Extract the fingerprint/hash part (the 40-character hex string)
    DEV_INSTALLER_IDENTITY=$(echo "$DEV_INSTALLER_IDENTITY" | grep -o -E '[0-9A-F]{40}')
    echo "Extracted installer identity hash: $DEV_INSTALLER_IDENTITY"
fi

# PACE Configuration
PACE_ACCOUNT="${PACE_ACCOUNT:-}"
PACE_ACCOUNT_PWD="${PACE_ACCOUNT_PWD:-}"
PACE_RENDERER_WCGUID="${PACE_RENDERER_WCGUID:-}"

# Check for both old and new variable names
# If PACE_AUDIOELEMENT_WCGUID is not set, use PACE_PANNER_WCGUID as fallback
PACE_AUDIOELEMENT_WCGUID="${PACE_AUDIOELEMENT_WCGUID:-${PACE_PANNER_WCGUID:-}}"
WRAP_TOOL="/Applications/PACEAntiPiracy/Eden/Fusion/Versions/5/bin/wraptool"

echo "--- Configuration ---"
echo "BUILD_CONFIG: $BUILD_CONFIG"
echo "BRANCH_NAME: $BRANCH_NAME"
echo "PLUGIN_FORMAT: $PLUGIN_FORMAT (FORMAT_SUFFIX: $FORMAT_SUFFIX)"
echo "SKIP_PACE_WRAPPING: $SKIP_PACE_WRAPPING"
echo "DEV_APP_SIGNING_IDENTITY: $DEV_APP_SIGNING_IDENTITY"
echo "DEV_INSTALLER_IDENTITY: $DEV_INSTALLER_IDENTITY"
echo "APPLE_TEAM_ID: $APPLE_TEAM_ID"
echo "APPLE_ACCOUNT_EMAIL: $APPLE_ACCOUNT_EMAIL"
echo "KEYCHAIN_PATH: $KEYCHAIN_PATH"
if [[ "$PLUGIN_FORMAT" == "aax" ]] && [ "$SKIP_PACE_WRAPPING" != "true" ]; then
    echo "PACE_ACCOUNT: $PACE_ACCOUNT"
    echo "PACE_ACCOUNT_PWD: [REDACTED]"
    echo "PACE_RENDERER_WCGUID: $PACE_RENDERER_WCGUID"
    echo "PACE_AUDIOELEMENT_WCGUID: $PACE_AUDIOELEMENT_WCGUID"
    [ -x "$WRAP_TOOL" ] || { echo >&2 "Error: PACE wraptool not found or not executable at $WRAP_TOOL."; exit 1; }
    [ -n "$PACE_ACCOUNT" ] || { echo >&2 "Error: PACE_ACCOUNT env var not set."; exit 1; }
    [ -n "$PACE_ACCOUNT_PWD" ] || { echo >&2 "Error: PACE_ACCOUNT_PWD env var not set."; exit 1; }
    [ -n "$PACE_RENDERER_WCGUID" ] || { echo >&2 "Error: PACE_RENDERER_WCGUID env var not set."; exit 1; }
    [ -n "$PACE_AUDIOELEMENT_WCGUID" ] || { echo >&2 "Error: PACE_AUDIOELEMENT_WCGUID env var not set."; exit 1; }
fi
echo "---------------------"

# --- Tool Checks ---
command -v codesign >/dev/null 2>&1 || { echo >&2 "Error: codesign not found."; exit 1; }
command -v security >/dev/null 2>&1 || { echo >&2 "Error: security not found."; exit 1; }
command -v install_name_tool >/dev/null 2>&1 || { echo >&2 "Error: install_name_tool not found."; exit 1; }
command -v otool >/dev/null 2>&1 || { echo >&2 "Error: otool not found."; exit 1; }
command -v pkgbuild >/dev/null 2>&1 || { echo >&2 "Error: pkgbuild not found."; exit 1; }
command -v productbuild >/dev/null 2>&1 || { echo >&2 "Error: productbuild not found."; exit 1; }
command -v xcrun >/dev/null 2>&1 || { echo >&2 "Error: xcrun not found."; exit 1; }

# --- Path Definitions ---
# AAX Plugin Paths
AAX_RENDERER_PLUGIN_SRC="build/rendererplugin/RendererPlugin_artefacts/$BUILD_CONFIG/AAX/Eclipsa Audio Renderer.aaxplugin"
AAX_AUDIOELEMENT_PLUGIN_SRC="build/audioelementplugin/AudioElementPlugin_artefacts/$BUILD_CONFIG/AAX/Eclipsa Audio Element Plugin.aaxplugin"
AAX_RENDERER_PLUGIN_BINARY="$AAX_RENDERER_PLUGIN_SRC/Contents/MacOS/Eclipsa Audio Renderer"
AAX_AUDIOELEMENT_PLUGIN_BINARY="$AAX_AUDIOELEMENT_PLUGIN_SRC/Contents/MacOS/Eclipsa Audio Element Plugin"
AAX_RENDERER_PLUGIN_RESOURCES="$AAX_RENDERER_PLUGIN_SRC/Contents/Resources"
AAX_AUDIOELEMENT_PLUGIN_RESOURCES="$AAX_AUDIOELEMENT_PLUGIN_SRC/Contents/Resources"

# VST3 Plugin Paths
VST3_RENDERER_PLUGIN_SRC="build/rendererplugin/RendererPlugin_artefacts/$BUILD_CONFIG/VST3/Eclipsa Audio Renderer.vst3"
VST3_AUDIOELEMENT_PLUGIN_SRC="build/audioelementplugin/AudioElementPlugin_artefacts/$BUILD_CONFIG/VST3/Eclipsa Audio Element Plugin.vst3"
VST3_RENDERER_PLUGIN_BINARY="$VST3_RENDERER_PLUGIN_SRC/Contents/MacOS/Eclipsa Audio Renderer"
VST3_AUDIOELEMENT_PLUGIN_BINARY="$VST3_AUDIOELEMENT_PLUGIN_SRC/Contents/MacOS/Eclipsa Audio Element Plugin"
VST3_RENDERER_PLUGIN_RESOURCES="$VST3_RENDERER_PLUGIN_SRC/Contents/Resources"
VST3_AUDIOELEMENT_PLUGIN_RESOURCES="$VST3_AUDIOELEMENT_PLUGIN_SRC/Contents/Resources"

# AU Plugin Paths
AU_RENDERER_PLUGIN_SRC="build/rendererplugin/RendererPlugin_artefacts/$BUILD_CONFIG/AU/Eclipsa Audio Renderer.component"
AU_AUDIOELEMENT_PLUGIN_SRC="build/audioelementplugin/AudioElementPlugin_artefacts/$BUILD_CONFIG/AU/Eclipsa Audio Element Plugin.component"
AU_RENDERER_PLUGIN_BINARY="$AU_RENDERER_PLUGIN_SRC/Contents/MacOS/Eclipsa Audio Renderer"
AU_AUDIOELEMENT_PLUGIN_BINARY="$AU_AUDIOELEMENT_PLUGIN_SRC/Contents/MacOS/Eclipsa Audio Element Plugin"
AU_RENDERER_PLUGIN_RESOURCES="$AU_RENDERER_PLUGIN_SRC/Contents/Resources"
AU_AUDIOELEMENT_PLUGIN_RESOURCES="$AU_AUDIOELEMENT_PLUGIN_SRC/Contents/Resources"

# Signing Directories
WRAPPED_OUTPUT_DIR="build/wrapped_plugins"
AAX_RENDERER_PLUGIN_SIGNING_DIR="$WRAPPED_OUTPUT_DIR/AAX"
AAX_AUDIOELEMENT_PLUGIN_SIGNING_DIR="$WRAPPED_OUTPUT_DIR/AAX"
VST3_RENDERER_PLUGIN_SIGNING_DIR="$WRAPPED_OUTPUT_DIR/VST3"
VST3_AUDIOELEMENT_PLUGIN_SIGNING_DIR="$WRAPPED_OUTPUT_DIR/VST3"
AU_RENDERER_PLUGIN_SIGNING_DIR="$WRAPPED_OUTPUT_DIR/AU"
AU_AUDIOELEMENT_PLUGIN_SIGNING_DIR="$WRAPPED_OUTPUT_DIR/AU"

AAX_RENDERER_PLUGIN_SIGNED="$AAX_RENDERER_PLUGIN_SIGNING_DIR/Eclipsa Audio Renderer.aaxplugin"
AAX_AUDIOELEMENT_PLUGIN_SIGNED="$AAX_AUDIOELEMENT_PLUGIN_SIGNING_DIR/Eclipsa Audio Element Plugin.aaxplugin"
VST3_RENDERER_PLUGIN_SIGNED="$VST3_RENDERER_PLUGIN_SIGNING_DIR/Eclipsa Audio Renderer.vst3"
VST3_AUDIOELEMENT_PLUGIN_SIGNED="$VST3_AUDIOELEMENT_PLUGIN_SIGNING_DIR/Eclipsa Audio Element Plugin.vst3"
AU_RENDERER_PLUGIN_SIGNED="$AU_RENDERER_PLUGIN_SIGNING_DIR/Eclipsa Audio Renderer.component"
AU_AUDIOELEMENT_PLUGIN_SIGNED="$AU_AUDIOELEMENT_PLUGIN_SIGNING_DIR/Eclipsa Audio Element Plugin.component"

# Packaging
PACKAGING_STAGING_DIR="./packaging_stage"
# Check for LICENSE.txt first, fall back to LICENSE if .txt version doesn't exist
if [ -f "./LICENSE.txt" ]; then
    LICENSE_FILE="./LICENSE.txt"
elif [ -f "./LICENSE" ]; then
    LICENSE_FILE="./LICENSE"
else
    echo "Error: License file not found. Looked for ./LICENSE.txt and ./LICENSE"
    exit 1
fi
INSTALLER_OUTPUT_DIR="build/installers"
INSTALLER_NAME="Eclipsa_${FORMAT_SUFFIX}_${BRANCH_NAME}.pkg"
FINAL_INSTALLER_PATH="$INSTALLER_OUTPUT_DIR/$INSTALLER_NAME"
# Set distribution XML path based on plugin format
if [ "$PLUGIN_FORMAT" = "aax" ]; then
    DISTRIBUTION_XML="./scripts/distribution_aax.xml"
elif [ "$PLUGIN_FORMAT" = "vst3" ]; then
    DISTRIBUTION_XML="./scripts/distribution_vst3.xml"
elif [ "$PLUGIN_FORMAT" = "au" ]; then
    DISTRIBUTION_XML="./scripts/distribution_au.xml"
fi
COMPONENT_PKG_DIR="build/component_pkgs"
COMPONENT_PKG_PATH="$COMPONENT_PKG_DIR/EclipsaPlugins.pkg"

# DMG Creation
FINAL_DMG_NAME="Eclipsa_Plugins_${FORMAT_SUFFIX}_${BRANCH_NAME}.dmg"
FINAL_DMG_PATH="$INSTALLER_OUTPUT_DIR/$FINAL_DMG_NAME"
DMG_TITLE="Eclipsa Plugins Installer"
DMG_STAGING_DIR="./dmg_staging"
TEMP_DMG_NAME="./temp_$FINAL_DMG_NAME"
DMG_WINDOW_SIZE="{300, 100, 1000, 598}"  # {x, y, width, height}
# Find the custom icon file in either current directory or scripts directory
if [ -f "./custom_installer_icon.png" ]; then
    CUSTOM_ICON_FILE="./custom_installer_icon.png"
elif [ -f "./scripts/custom_installer_icon.png" ]; then
    CUSTOM_ICON_FILE="./scripts/custom_installer_icon.png"
else
    # Not critical, will use default icon if not found
    CUSTOM_ICON_FILE=""
fi

# Find the DMG background file in either current directory or scripts directory
if [ -f "./ECLIPSA_badge_ic_H_4200x952px_GREY-POS@2x.png" ]; then
    DMG_BACKGROUND_FILE="./ECLIPSA_badge_ic_H_4200x952px_GREY-POS@2x.png"
elif [ -f "./scripts/ECLIPSA_badge_ic_H_4200x952px_GREY-POS@2x.png" ]; then
    DMG_BACKGROUND_FILE="./scripts/ECLIPSA_badge_ic_H_4200x952px_GREY-POS@2x.png"
else
    # Not critical, will use a solid color background if not found
    DMG_BACKGROUND_FILE=""
fi
RESIZED_BACKGROUND="./dmg_background.png"
MOUNT_POINT="/Volumes/$DMG_TITLE"
TMP_ICONSET="./tmp.iconset"

# --- Helper Functions ---
unlock_keychain() {
    echo "Attempting to unlock keychain: $KEYCHAIN_PATH"
    if [ -n "$KEYCHAIN_PASSWORD" ]; then
        security unlock-keychain -p "$KEYCHAIN_PASSWORD" "$KEYCHAIN_PATH" || { echo "Failed to unlock keychain."; exit 1; }
    else
        security find-identity -v "$KEYCHAIN_PATH" > /dev/null || { echo "Failed to access keychain $KEYCHAIN_PATH"; exit 1; }
    fi
    security default-keychain -s "$KEYCHAIN_PATH"
    security list-keychains -d user -s login.keychain "$KEYCHAIN_PATH"
    echo "Keychain unlocked and set as default."
}

sign_file_dev_app() {
    local file_path="$1"
    local identity="$DEV_APP_SIGNING_IDENTITY"
    if [ ! -e "$file_path" ]; then
        echo "Warning: File to sign does not exist: $file_path. Skipping."
        return
    fi
    echo "Signing file (Dev App ID): $file_path"
    codesign --keychain "$KEYCHAIN_PATH" \
             --sign "$identity" \
             --timestamp \
             --force \
             --options runtime \
             "$file_path" || { echo "Failed to sign file: $file_path"; exit 1; }
    echo "Successfully signed: $file_path"
}

adjust_rpath() {
    local target_file="$1"
    if [ ! -f "$target_file" ]; then
        echo "Warning: File $target_file does not exist for RPATH adjustment. Skipping."
        return
    fi
    echo "Adjusting RPATH for file: $target_file"
    RPATH_TO_REMOVE=$(otool -l "$target_file" | grep -A 1 LC_RPATH | grep ' path ' | grep 'zeromq-build/lib' | sed -n 's/ *path \(.*\) (offset.*/\1/p' | head -n 1)
    if [ -n "$RPATH_TO_REMOVE" ]; then
        echo "Found ZMQ RPATH: $RPATH_TO_REMOVE"
        install_name_tool -delete_rpath "$RPATH_TO_REMOVE" "$target_file" || { echo "Failed to remove RPATH from $target_file"; exit 1; }
        echo "Removed RPATH: $RPATH_TO_REMOVE from $target_file"
    else
        echo "No ZMQ RPATH found in $target_file, skipping RPATH removal."
    fi
}

# --- Main Script ---
echo "Starting CI process..."

# --- Pre-Checks ---
if [ ! -f "$LICENSE_FILE" ]; then echo "Error: License file not found at $LICENSE_FILE"; exit 1; fi
if [ ! -f "$DISTRIBUTION_XML" ]; then echo "Error: Distribution XML not found at $DISTRIBUTION_XML"; exit 1; fi

# Check plugin paths based on format
if [[ "$PLUGIN_FORMAT" == "aax" ]]; then
    if [ ! -d "$AAX_RENDERER_PLUGIN_SRC" ]; then echo "Error: AAX Renderer plugin source not found at $AAX_RENDERER_PLUGIN_SRC"; exit 1; fi
    if [ ! -d "$AAX_AUDIOELEMENT_PLUGIN_SRC" ]; then echo "Error: AAX AudioElement plugin source not found at $AAX_AUDIOELEMENT_PLUGIN_SRC"; exit 1; fi
fi

if [[ "$PLUGIN_FORMAT" == "vst3" ]]; then
    if [ ! -d "$VST3_RENDERER_PLUGIN_SRC" ]; then echo "Error: VST3 Renderer plugin source not found at $VST3_RENDERER_PLUGIN_SRC"; exit 1; fi
    if [ ! -d "$VST3_AUDIOELEMENT_PLUGIN_SRC" ]; then echo "Error: VST3 AudioElement plugin source not found at $VST3_AUDIOELEMENT_PLUGIN_SRC"; exit 1; fi
fi

if [[ "$PLUGIN_FORMAT" == "au" ]]; then
    if [ ! -d "$AU_RENDERER_PLUGIN_SRC" ]; then echo "Error: AU Renderer plugin source not found at $AU_RENDERER_PLUGIN_SRC"; exit 1; fi
    if [ ! -d "$AU_AUDIOELEMENT_PLUGIN_SRC" ]; then echo "Error: AU AudioElement plugin source not found at $AU_AUDIOELEMENT_PLUGIN_SRC"; exit 1; fi
fi

# Check DMG-related files
if [ -f "$CUSTOM_ICON_FILE" ]; then
    echo "Custom DMG icon found at $CUSTOM_ICON_FILE"
else
    echo "Warning: Custom DMG icon not found at $CUSTOM_ICON_FILE"
fi

if [ -f "$DMG_BACKGROUND_FILE" ]; then
    echo "DMG background image found at $DMG_BACKGROUND_FILE"
else
    echo "Warning: DMG background image not found at $DMG_BACKGROUND_FILE"
fi

# 1. Unlock Keychain
unlock_keychain

# 2. Adjust RPATH and sign dylibs based on selected plugin format
echo "Adjusting RPATH and signing dylibs for selected plugin formats..."

if [[ "$PLUGIN_FORMAT" == "aax" ]]; then
    # Adjust RPATH for AAX plugins
    echo "Processing AAX plugins..."
    adjust_rpath "$AAX_RENDERER_PLUGIN_BINARY"
    adjust_rpath "$AAX_AUDIOELEMENT_PLUGIN_BINARY"

    # Sign internal AAX dylibs
    echo "Signing AAX internal dylibs..."
    find "$AAX_RENDERER_PLUGIN_RESOURCES" -name '*.dylib' -print0 | while IFS= read -r -d $'\0' dylib; do sign_file_dev_app "$dylib"; done
    find "$AAX_AUDIOELEMENT_PLUGIN_RESOURCES" -name '*.dylib' -print0 | while IFS= read -r -d $'\0' dylib; do sign_file_dev_app "$dylib"; done
fi

if [[ "$PLUGIN_FORMAT" == "vst3" ]]; then
    # Adjust RPATH for VST3 plugins
    echo "Processing VST3 plugins..."
    adjust_rpath "$VST3_RENDERER_PLUGIN_BINARY"
    adjust_rpath "$VST3_AUDIOELEMENT_PLUGIN_BINARY"

    # Sign internal VST3 dylibs
    echo "Signing VST3 internal dylibs..."
    find "$VST3_RENDERER_PLUGIN_RESOURCES" -name '*.dylib' -print0 | while IFS= read -r -d $'\0' dylib; do sign_file_dev_app "$dylib"; done
    find "$VST3_AUDIOELEMENT_PLUGIN_RESOURCES" -name '*.dylib' -print0 | while IFS= read -r -d $'\0' dylib; do sign_file_dev_app "$dylib"; done
fi

if [[ "$PLUGIN_FORMAT" == "au" ]]; then
    # Adjust RPATH for AU plugins
    echo "Processing AU plugins..."
    adjust_rpath "$AU_RENDERER_PLUGIN_BINARY"
    adjust_rpath "$AU_AUDIOELEMENT_PLUGIN_BINARY"

    # Sign internal AU dylibs
    echo "Signing AU internal dylibs..."
    find "$AU_RENDERER_PLUGIN_RESOURCES" -name '*.dylib' -print0 | while IFS= read -r -d $'\0' dylib; do sign_file_dev_app "$dylib"; done
    find "$AU_AUDIOELEMENT_PLUGIN_RESOURCES" -name '*.dylib' -print0 | while IFS= read -r -d $'\0' dylib; do sign_file_dev_app "$dylib"; done
fi

# 3. Sign and process plugins based on format
echo "Processing and signing plugins..."

# Create output directories
echo "Creating output directories..."
rm -rf "$WRAPPED_OUTPUT_DIR"
mkdir -p "$WRAPPED_OUTPUT_DIR"
mkdir -p "$AAX_RENDERER_PLUGIN_SIGNING_DIR"
mkdir -p "$AAX_AUDIOELEMENT_PLUGIN_SIGNING_DIR"
mkdir -p "$VST3_RENDERER_PLUGIN_SIGNING_DIR"
mkdir -p "$VST3_AUDIOELEMENT_PLUGIN_SIGNING_DIR"
mkdir -p "$AU_RENDERER_PLUGIN_SIGNING_DIR"
mkdir -p "$AU_AUDIOELEMENT_PLUGIN_SIGNING_DIR"

# Process AAX Plugins
if [[ "$PLUGIN_FORMAT" == "aax" ]]; then
    echo "Processing AAX plugins with PACE wrapping: $SKIP_PACE_WRAPPING"
    
    if [ "$SKIP_PACE_WRAPPING" != "true" ]; then
        # Use PACE wraptool for AAX plugins
        echo "Running PACE wraptool for AAX plugins..."
        "$WRAP_TOOL" sign --verbose --account "$PACE_ACCOUNT" --password "$PACE_ACCOUNT_PWD" --wcguid "$PACE_RENDERER_WCGUID" \
                         --in "$AAX_RENDERER_PLUGIN_SRC" --out "$AAX_RENDERER_PLUGIN_SIGNED" \
                         --signid "$DEV_APP_SIGNING_IDENTITY_FULL" \
                         --dsigharden --autoinstall on || { echo "Failed to wrap AAX Renderer plugin"; exit 1; }
                         
        "$WRAP_TOOL" sign --verbose --account "$PACE_ACCOUNT" --password "$PACE_ACCOUNT_PWD" --wcguid "$PACE_AUDIOELEMENT_WCGUID" \
                         --in "$AAX_AUDIOELEMENT_PLUGIN_SRC" --out "$AAX_AUDIOELEMENT_PLUGIN_SIGNED" \
                         --signid "$DEV_APP_SIGNING_IDENTITY_FULL" \
                         --dsigharden --autoinstall on || { echo "Failed to wrap AAX AudioElement plugin"; exit 1; }
        echo "PACE wrapping for AAX plugins completed."
    else
        # Standard Apple signing for AAX plugins
        echo "Using standard Apple signing for AAX plugins..."
        sign_file_dev_app "$AAX_RENDERER_PLUGIN_BINARY"
        sign_file_dev_app "$AAX_AUDIOELEMENT_PLUGIN_BINARY"
        
        # Copy to output directories
        cp -R "$AAX_RENDERER_PLUGIN_SRC" "$AAX_RENDERER_PLUGIN_SIGNED" || { echo "Failed to copy AAX Renderer plugin"; exit 1; }
        cp -R "$AAX_AUDIOELEMENT_PLUGIN_SRC" "$AAX_AUDIOELEMENT_PLUGIN_SIGNED" || { echo "Failed to copy AAX AudioElement plugin"; exit 1; }
    fi
    
    # Verify AAX plugins were processed
    if [ ! -d "$AAX_RENDERER_PLUGIN_SIGNED" ]; then echo "Error: AAX Renderer plugin not found at $AAX_RENDERER_PLUGIN_SIGNED"; exit 1; fi
    if [ ! -d "$AAX_AUDIOELEMENT_PLUGIN_SIGNED" ]; then echo "Error: AAX AudioElement plugin not found at $AAX_AUDIOELEMENT_PLUGIN_SIGNED"; exit 1; fi
    echo "AAX plugin processing completed."
fi

# Process VST3 Plugins
if [[ "$PLUGIN_FORMAT" == "vst3" ]]; then
    echo "Processing VST3 plugins..."
    
    # Copy to output directories first
    mkdir -p "$(dirname "$VST3_RENDERER_PLUGIN_SIGNED")"
    mkdir -p "$(dirname "$VST3_AUDIOELEMENT_PLUGIN_SIGNED")"
    cp -R "$VST3_RENDERER_PLUGIN_SRC" "$VST3_RENDERER_PLUGIN_SIGNED" || { echo "Failed to copy VST3 Renderer plugin"; exit 1; }
    cp -R "$VST3_AUDIOELEMENT_PLUGIN_SRC" "$VST3_AUDIOELEMENT_PLUGIN_SIGNED" || { echo "Failed to copy VST3 AudioElement plugin"; exit 1; }
    
    # Create entitlements file for hardened runtime
    entitlements_file=$(mktemp)
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
</dict>
</plist>
EOF
    
    # Sign VST3 components in the right order: dylibs first, then executables, then the bundle itself
    echo "Signing VST3 Renderer plugin components..."
    # Sign all frameworks and dylibs first
    find "$VST3_RENDERER_PLUGIN_SIGNED" -type f -name "*.dylib" | while read -r file; do
        sign_file_dev_app "$file"
    done
    
    # Sign all executable binaries
    find "$VST3_RENDERER_PLUGIN_SIGNED/Contents/MacOS" -type f | while read -r file; do
        codesign --keychain "$KEYCHAIN_PATH" \
                 --sign "$DEV_APP_SIGNING_IDENTITY" \
                 --timestamp \
                 --force \
                 --options runtime \
                 --entitlements "$entitlements_file" \
                 "$file" || { echo "Failed to sign binary: $file"; exit 1; }
        echo "Successfully signed binary: $file"
    done
    
    # Sign the main bundle
    codesign --keychain "$KEYCHAIN_PATH" \
             --sign "$DEV_APP_SIGNING_IDENTITY" \
             --timestamp \
             --force \
             --options runtime \
             --entitlements "$entitlements_file" \
             --deep \
             "$VST3_RENDERER_PLUGIN_SIGNED" || { echo "Failed to sign VST3 Renderer bundle"; exit 1; }
    
    echo "Signing VST3 AudioElement plugin components..."
    # Sign all frameworks and dylibs first
    find "$VST3_AUDIOELEMENT_PLUGIN_SIGNED" -type f -name "*.dylib" | while read -r file; do
        sign_file_dev_app "$file"
    done
    
    # Sign all executable binaries
    find "$VST3_AUDIOELEMENT_PLUGIN_SIGNED/Contents/MacOS" -type f | while read -r file; do
        codesign --keychain "$KEYCHAIN_PATH" \
                 --sign "$DEV_APP_SIGNING_IDENTITY" \
                 --timestamp \
                 --force \
                 --options runtime \
                 --entitlements "$entitlements_file" \
                 "$file" || { echo "Failed to sign binary: $file"; exit 1; }
        echo "Successfully signed binary: $file"
    done
    
    # Sign the main bundle
    codesign --keychain "$KEYCHAIN_PATH" \
             --sign "$DEV_APP_SIGNING_IDENTITY" \
             --timestamp \
             --force \
             --options runtime \
             --entitlements "$entitlements_file" \
             --deep \
             "$VST3_AUDIOELEMENT_PLUGIN_SIGNED" || { echo "Failed to sign VST3 AudioElement bundle"; exit 1; }
    
    # Clean up
    rm -f "$entitlements_file"
    
    # Verify VST3 plugins were processed
    if [ ! -d "$VST3_RENDERER_PLUGIN_SIGNED" ]; then echo "Error: VST3 Renderer plugin not found at $VST3_RENDERER_PLUGIN_SIGNED"; exit 1; fi
    if [ ! -d "$VST3_AUDIOELEMENT_PLUGIN_SIGNED" ]; then echo "Error: VST3 AudioElement plugin not found at $VST3_AUDIOELEMENT_PLUGIN_SIGNED"; exit 1; fi
    
    # Verify signatures
    echo "Verifying VST3 plugin signatures..."
    codesign --verify --deep --strict --verbose=2 "$VST3_RENDERER_PLUGIN_SIGNED" || echo "Warning: VST3 Renderer signature verification had issues but continuing..."
    codesign --verify --deep --strict --verbose=2 "$VST3_AUDIOELEMENT_PLUGIN_SIGNED" || echo "Warning: VST3 AudioElement signature verification had issues but continuing..."
    
    echo "VST3 plugin processing completed."
fi

# Process au Plugins
if [[ "$PLUGIN_FORMAT" == "au" ]]; then
    echo "Processing au plugins..."
    
    # Copy to output directories first
    mkdir -p "$(dirname "$AU_RENDERER_PLUGIN_SIGNED")"
    mkdir -p "$(dirname "$AU_AUDIOELEMENT_PLUGIN_SIGNED")"
    cp -R "$AU_RENDERER_PLUGIN_SRC" "$AU_RENDERER_PLUGIN_SIGNED" || { echo "Failed to copy AU Renderer plugin"; exit 1; }
    cp -R "$AU_AUDIOELEMENT_PLUGIN_SRC" "$AU_AUDIOELEMENT_PLUGIN_SIGNED" || { echo "Failed to copy AU AudioElement plugin"; exit 1; }
    
    # Create entitlements file for hardened runtime
    entitlements_file=$(mktemp)
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
</dict>
</plist>
EOF
    
    # Sign AU components in the right order: dylibs first, then executables, then the bundle itself
    echo "Signing AU Renderer plugin components..."
    # Sign all frameworks and dylibs first
    find "$AU_RENDERER_PLUGIN_SIGNED" -type f -name "*.dylib" | while read -r file; do
        sign_file_dev_app "$file"
    done
    
    # Sign all executable binaries
    find "$AU_RENDERER_PLUGIN_SIGNED/Contents/MacOS" -type f | while read -r file; do
        codesign --keychain "$KEYCHAIN_PATH" \
                 --sign "$DEV_APP_SIGNING_IDENTITY" \
                 --timestamp \
                 --force \
                 --options runtime \
                 --entitlements "$entitlements_file" \
                 "$file" || { echo "Failed to sign binary: $file"; exit 1; }
        echo "Successfully signed binary: $file"
    done
    
    # Sign the main bundle
    codesign --keychain "$KEYCHAIN_PATH" \
             --sign "$DEV_APP_SIGNING_IDENTITY" \
             --timestamp \
             --force \
             --options runtime \
             --entitlements "$entitlements_file" \
             --deep \
             "$AU_RENDERER_PLUGIN_SIGNED" || { echo "Failed to sign AU Renderer bundle"; exit 1; }
    
    echo "Signing AU AudioElement plugin components..."
    # Sign all frameworks and dylibs first
    find "$AU_AUDIOELEMENT_PLUGIN_SIGNED" -type f -name "*.dylib" | while read -r file; do
        sign_file_dev_app "$file"
    done
    
    # Sign all executable binaries
    find "$AU_AUDIOELEMENT_PLUGIN_SIGNED/Contents/MacOS" -type f | while read -r file; do
        codesign --keychain "$KEYCHAIN_PATH" \
                 --sign "$DEV_APP_SIGNING_IDENTITY" \
                 --timestamp \
                 --force \
                 --options runtime \
                 --entitlements "$entitlements_file" \
                 "$file" || { echo "Failed to sign binary: $file"; exit 1; }
        echo "Successfully signed binary: $file"
    done
    
    # Sign the main bundle
    codesign --keychain "$KEYCHAIN_PATH" \
             --sign "$DEV_APP_SIGNING_IDENTITY" \
             --timestamp \
             --force \
             --options runtime \
             --entitlements "$entitlements_file" \
             --deep \
             "$AU_AUDIOELEMENT_PLUGIN_SIGNED" || { echo "Failed to sign AU AudioElement bundle"; exit 1; }
    
    # Clean up
    rm -f "$entitlements_file"
    
    # Verify AU plugins were processed
    if [ ! -d "$AU_RENDERER_PLUGIN_SIGNED" ]; then echo "Error: AU Renderer plugin not found at $AU_RENDERER_PLUGIN_SIGNED"; exit 1; fi
    if [ ! -d "$AU_AUDIOELEMENT_PLUGIN_SIGNED" ]; then echo "Error: AU AudioElement plugin not found at $AU_AUDIOELEMENT_PLUGIN_SIGNED"; exit 1; fi
    
    # Verify signatures
    echo "Verifying AU plugin signatures..."
    codesign --verify --deep --strict --verbose=2 "$AU_RENDERER_PLUGIN_SIGNED" || echo "Warning: AU Renderer signature verification had issues but continuing..."
    codesign --verify --deep --strict --verbose=2 "$AU_AUDIOELEMENT_PLUGIN_SIGNED" || echo "Warning: AU AudioElement signature verification had issues but continuing..."
    
    echo "AU plugin processing completed."
fi

# 4. Prepare staging directory for PKG installer
echo "Preparing files for packaging..."
rm -rf "$PACKAGING_STAGING_DIR"

# Create different staging structures based on plugin format
if [[ "$PLUGIN_FORMAT" == "aax" ]]; then
    # AAX plugins must be installed system-wide
    mkdir -p "$PACKAGING_STAGING_DIR/Library/Application Support/Eclipsa Audio Plugins"
    mkdir -p "$PACKAGING_STAGING_DIR/Library/Application Support/Avid/Audio/Plug-Ins"
    echo "Adding AAX plugins to package..."
    cp -R "$AAX_RENDERER_PLUGIN_SIGNED" "$AAX_AUDIOELEMENT_PLUGIN_SIGNED" "$PACKAGING_STAGING_DIR/Library/Application Support/Avid/Audio/Plug-Ins/" || { echo "Failed to copy AAX plugins"; exit 1; }
elif [[ "$PLUGIN_FORMAT" == "vst3" ]]; then
    # VST3 plugins structure - use staging area only, postinstall will handle final placement
    mkdir -p "$PACKAGING_STAGING_DIR/Library/Application Support/Eclipsa Audio Plugins"
    mkdir -p "$PACKAGING_STAGING_DIR/tmp/EclipsaVST3"
    echo "Adding VST3 plugins to package (staging area only)..."
    # Copy ONLY to staging area - postinstall will handle final placement to avoid permission conflicts
    cp -R "$VST3_RENDERER_PLUGIN_SIGNED" "$VST3_AUDIOELEMENT_PLUGIN_SIGNED" "$PACKAGING_STAGING_DIR/tmp/EclipsaVST3/" || { echo "Failed to copy VST3 plugins to temp location"; exit 1; }
elif [[ "$PLUGIN_FORMAT" == "au" ]]; then
    # au plugins structure - use staging area only, postinstall will handle final placement
    mkdir -p "$PACKAGING_STAGING_DIR/Library/Application Support/Eclipsa Audio Plugins"
    mkdir -p "$PACKAGING_STAGING_DIR/tmp/EclipsaAU"
    echo "Adding AU plugins to package (staging area only)..."
    # Copy ONLY to staging area - postinstall will handle final placement to avoid permission conflicts
    cp -R "$AU_RENDERER_PLUGIN_SIGNED" "$AU_AUDIOELEMENT_PLUGIN_SIGNED" "$PACKAGING_STAGING_DIR/tmp/EclipsaAU/" || { echo "Failed to copy AU plugins to temp location"; exit 1; }
fi

# Copy license for all formats
cp "$LICENSE_FILE" "$PACKAGING_STAGING_DIR/Library/Application Support/Eclipsa Audio Plugins/" || { echo "Failed to copy license"; exit 1; }

echo "Preparing ownership and permissions for staging directory: $PACKAGING_STAGING_DIR"
CURRENT_USER_CI=$(whoami)
CURRENT_GROUP_CI=$(id -g -n "$CURRENT_USER_CI")
echo "Changing ownership of $PACKAGING_STAGING_DIR to $CURRENT_USER_CI:$CURRENT_GROUP_CI"
sudo chown -R "$CURRENT_USER_CI:$CURRENT_GROUP_CI" "$PACKAGING_STAGING_DIR"
echo "Setting permissions for $PACKAGING_STAGING_DIR to 755"
sudo chmod -R 755 "$PACKAGING_STAGING_DIR"

# 6. Build component package
echo "Building component package..."
rm -rf "$COMPONENT_PKG_DIR"
mkdir -p "$COMPONENT_PKG_DIR"

# Build the component package with appropriate scripts based on plugin format
if [[ "$PLUGIN_FORMAT" == "vst3" ]]; then
    # For VST3, create scripts directory with preinstall and postinstall scripts
    SCRIPTS_DIR="./vst3_scripts_ci"
    mkdir -p "$SCRIPTS_DIR"
    
    # Find the VST3 preinstall script
    if [ -f "./preinstall_vst3" ]; then
        VST3_PREINSTALL="./preinstall_vst3"
    elif [ -f "./scripts/preinstall_vst3" ]; then
        VST3_PREINSTALL="./scripts/preinstall_vst3"
    else
        echo "Error: VST3 preinstall script not found. Looked for ./preinstall_vst3 and ./scripts/preinstall_vst3"
        exit 1
    fi

    # Find the VST3 postinstall script
    if [ -f "./postinstall_vst3" ]; then
        VST3_POSTINSTALL="./postinstall_vst3"
    elif [ -f "./scripts/postinstall_vst3" ]; then
        VST3_POSTINSTALL="./scripts/postinstall_vst3"
    else
        echo "Error: VST3 postinstall script not found. Looked for ./postinstall_vst3 and ./scripts/postinstall_vst3"
        exit 1
    fi
    
    # Copy VST3 preinstall and postinstall scripts
    cp "$VST3_PREINSTALL" "$SCRIPTS_DIR/preinstall"
    chmod +x "$SCRIPTS_DIR/preinstall"
    cp "$VST3_POSTINSTALL" "$SCRIPTS_DIR/postinstall"
    chmod +x "$SCRIPTS_DIR/postinstall"
    
    echo "Building VST3 component package with preinstall and postinstall scripts..."
    pkgbuild --root "$PACKAGING_STAGING_DIR" \
        --identifier "com.eclipsaproject.plugins" \
        --install-location "/" \
        --scripts "$SCRIPTS_DIR" \
        --version "1.0.0" \
        --sign "$DEV_INSTALLER_IDENTITY" \
        --preserve-xattr \
        "$COMPONENT_PKG_PATH" || { echo "pkgbuild failed"; exit 1; }
    
    # Clean up temporary scripts directory
    rm -rf "$SCRIPTS_DIR"
elif [[ "$PLUGIN_FORMAT" == "au" ]]; then
    # For AU, create scripts directory with preinstall and postinstall scripts
    SCRIPTS_DIR="./au_scripts_ci"
    mkdir -p "$SCRIPTS_DIR"
    
    # Find the AU preinstall script
    if [ -f "./preinstall_au" ]; then
        AU_PREINSTALL="./preinstall_au"
    elif [ -f "./scripts/preinstall_au" ]; then
        AU_PREINSTALL="./scripts/preinstall_au"
    else
        echo "Error: AU preinstall script not found. Looked for ./preinstall_au and ./scripts/preinstall_au"
        exit 1
    fi

    # Find the AU postinstall script
    if [ -f "./postinstall_au" ]; then
        AU_POSTINSTALL="./postinstall_au"
    elif [ -f "./scripts/postinstall_au" ]; then
        AU_POSTINSTALL="./scripts/postinstall_au"
    else
        echo "Error: AU postinstall script not found. Looked for ./postinstall_au and ./scripts/postinstall_au"
        exit 1
    fi
    
    # Copy AU preinstall and postinstall scripts
    cp "$AU_PREINSTALL" "$SCRIPTS_DIR/preinstall"
    chmod +x "$SCRIPTS_DIR/preinstall"
    cp "$AU_POSTINSTALL" "$SCRIPTS_DIR/postinstall"
    chmod +x "$SCRIPTS_DIR/postinstall"
    
    echo "Building AU component package with preinstall and postinstall scripts..."
    pkgbuild --root "$PACKAGING_STAGING_DIR" \
        --identifier "com.eclipsaproject.plugins" \
        --install-location "/" \
        --scripts "$SCRIPTS_DIR" \
        --version "1.0.0" \
        --sign "$DEV_INSTALLER_IDENTITY" \
        --preserve-xattr \
        "$COMPONENT_PKG_PATH" || { echo "pkgbuild failed"; exit 1; }
    
    # Clean up temporary scripts directory
    rm -rf "$SCRIPTS_DIR"
else
    # For AAX, no postinstall script needed (direct installation)
    echo "Building AAX component package (no postinstall script)..."
    pkgbuild --root "$PACKAGING_STAGING_DIR" \
        --identifier "com.eclipsaproject.plugins" \
        --install-location "/" \
        --version "1.0.0" \
        --sign "$DEV_INSTALLER_IDENTITY" \
        --preserve-xattr \
        "$COMPONENT_PKG_PATH" || { echo "pkgbuild failed"; exit 1; }
fi

echo "Component package built."

# 7. Build and sign distribution package
echo "Building and signing distribution package..."
rm -rf "$INSTALLER_OUTPUT_DIR"
mkdir -p "$INSTALLER_OUTPUT_DIR"

# Make sure the LICENSE file is available at the location expected by distribution.xml
cp "$LICENSE_FILE" ./LICENSE 2>/dev/null || echo "License already in place."

# Create a temporary pkg file first
TEMP_PKG_PATH="$INSTALLER_OUTPUT_DIR/temp_installer.pkg"

productbuild --distribution "$DISTRIBUTION_XML" \
    --resources . \
    --package-path "$COMPONENT_PKG_DIR" \
    "$TEMP_PKG_PATH" || { echo "productbuild failed"; exit 1; }

# Apply custom icon to the PKG
if [ -f "$CUSTOM_ICON_FILE" ]; then
    echo "Creating PKG with custom icon..."
    
    # Create iconset directory 
    PKG_ICONS_TMP="./pkg_icons.iconset"
    mkdir -p "$PKG_ICONS_TMP"
    
    # Generate icons at different sizes
    for size in 16 32 128 256 512; do
        sips -z $size $size "$CUSTOM_ICON_FILE" --out "$PKG_ICONS_TMP/icon_${size}x${size}.png" > /dev/null 2>&1
        doubleSize=$((size * 2))
        sips -z $doubleSize $doubleSize "$CUSTOM_ICON_FILE" --out "$PKG_ICONS_TMP/icon_${size}x${size}@2x.png" > /dev/null 2>&1
    done
    
    # Convert iconset to icns
    PKG_ICON_PATH="./pkg_icon.icns"
    iconutil -c icns "$PKG_ICONS_TMP" -o "$PKG_ICON_PATH" || { echo "Warning: iconutil failed to create $PKG_ICON_PATH"; PKG_ICON_PATH=""; }

    if [ ! -f "$PKG_ICON_PATH" ]; then
        echo "Fallback: Trying sips to create .icns directly for PKG icon"
        sips -s format icns "$CUSTOM_ICON_FILE" --out "$PKG_ICON_PATH" || { echo "Warning: sips fallback for PKG .icns also failed"; PKG_ICON_PATH=""; }
    fi

    if [ ! -f "$PKG_ICON_PATH" ]; then
        echo "ERROR: Failed to create PKG icon file ($PKG_ICON_PATH) from $CUSTOM_ICON_FILE. Skipping custom PKG icon."
    else
        echo "Successfully created PKG icon: $PKG_ICON_PATH"
    fi
    
    # Extract the pkg for customization
    PKG_WORK_DIR="./tmp_pkg_work"
    mkdir -p "$PKG_WORK_DIR"
    
    # Extract the package
    echo "Extracting package for icon customization..."
    pkgutil --expand "$TEMP_PKG_PATH" "$PKG_WORK_DIR/expanded"
    
    # Add resources directory
    mkdir -p "$PKG_WORK_DIR/expanded/Resources" 
    
    # Copy the icon file for compatibility
    cp "$PKG_ICON_PATH" "$PKG_WORK_DIR/expanded/Resources/background"
    cp "$PKG_ICON_PATH" "$PKG_WORK_DIR/expanded/Resources/package.icns"
    
    # Add icon references to Distribution file
    if [ -f "$PKG_WORK_DIR/expanded/Distribution" ]; then
        if ! grep -q "background-image" "$PKG_WORK_DIR/expanded/Distribution"; then
            echo "Adding icon references to Distribution file"
            # Using sed similar to package.sh, ensuring background element is added
            sed -i '' 's|</installer-gui-script>|    <background file="background" alignment="topleft" scaling="none"/>\n    <background-darkAqua file="background" alignment="topleft" scaling="none"/>\n</installer-gui-script>|g' "$PKG_WORK_DIR/expanded/Distribution"
        fi
    fi
    
    # Flatten the package back into the original TEMP_PKG_PATH
    echo "Rebuilding package with custom icon..."
    pkgutil --flatten "$PKG_WORK_DIR/expanded" "$TEMP_PKG_PATH" # Flatten back to TEMP_PKG_PATH
    
    # Note: Rez/SetFile will be applied AFTER productsign, to $FINAL_INSTALLER_PATH
    # Clean up temporary files from pkgutil expand/flatten
    rm -rf "$PKG_ICONS_TMP" "$PKG_WORK_DIR"
else
    echo "No custom icon file found, using standard PKG appearance"
fi

# Now sign it with the installer identity
echo "Signing package with Developer ID Installer..."
productsign --keychain "$KEYCHAIN_PATH" \
    --sign "$DEV_INSTALLER_IDENTITY" \
    "$TEMP_PKG_PATH" \
    "$FINAL_INSTALLER_PATH" || { echo "productsign failed"; exit 1; }

# Now, apply the resource fork icon to the *signed* FINAL_INSTALLER_PATH
if [ -f "$CUSTOM_ICON_FILE" ] && [ -n "$PKG_ICON_PATH" ] && [ -f "$PKG_ICON_PATH" ]; then
    echo "Applying resource fork icon method to signed package: $FINAL_INSTALLER_PATH..."
    # Define RSRC_DEF path. Using a simple name in the current directory.
    FINAL_RSRC_DEF="./final_pkg_icon.rsrc"
    echo "read 'icns' (-16455) \"$PKG_ICON_PATH\";" > "$FINAL_RSRC_DEF"
    
    if command -v xcrun &> /dev/null && xcrun -f Rez &> /dev/null && xcrun -f SetFile &> /dev/null; then
        echo "Using Rez and SetFile to attach icon to $FINAL_INSTALLER_PATH..."
        xcrun Rez "$FINAL_RSRC_DEF" -o "$FINAL_INSTALLER_PATH" -append || echo "Warning: Rez command failed for $FINAL_INSTALLER_PATH but continuing..."
        xcrun SetFile -a C "$FINAL_INSTALLER_PATH" || echo "Warning: SetFile command failed for $FINAL_INSTALLER_PATH but continuing..."
    else
        echo "Warning: Rez or SetFile not found. Skipping resource fork icon application for $FINAL_INSTALLER_PATH."
    fi
    rm -f "$FINAL_RSRC_DEF" # Clean up the temp rsrc definition
else
    echo "Skipping final resource fork icon method for PKG as PKG_ICON_PATH is invalid or custom icon not specified."
fi

# Clean up the temporary $TEMP_PKG_PATH which was the input to productsign
rm -f "$TEMP_PKG_PATH"

# Verify the signature
echo "Verifying package signature..."
pkgutil --check-signature "$FINAL_INSTALLER_PATH" || echo "Warning: Package signature verification had issues but continuing..."

echo "Signed distribution installer created."

# 8. Notarize installer
echo "Submitting installer for notarization..."
SUBMISSION_OUTPUT=$(xcrun notarytool submit "$FINAL_INSTALLER_PATH" \
    --apple-id "$APPLE_ACCOUNT_EMAIL" \
    --team-id "$APPLE_TEAM_ID" \
    --password "$APPLE_APP_SPECIFIC_PASSWORD" \
    --output-format json 2>&1) || {
    echo "Error: notarytool submit command failed. Output: $SUBMISSION_OUTPUT"
    echo "Continuing without notarization..."
}

# Print the output for debugging
echo "Notarization submission output:"
echo "$SUBMISSION_OUTPUT"

# Try multiple ways to extract the ID
# First try JSON pattern that looks like "id": "value"
if echo "$SUBMISSION_OUTPUT" | grep -q '"id"'; then
    SUBMISSION_ID=$(echo "$SUBMISSION_OUTPUT" | grep -o '"id"[[:space:]]*:[[:space:]]*"[^"]*"' | sed 's/.*"id"[[:space:]]*:[[:space:]]*"\([^"]*\)".*/\1/')
# Then try JSON pattern that looks like "id" : "value" (with space around colon)
elif echo "$SUBMISSION_OUTPUT" | grep -q '"id"[[:space:]]*:[[:space:]]*"'; then
    SUBMISSION_ID=$(echo "$SUBMISSION_OUTPUT" | grep -o '"id"[[:space:]]*:[[:space:]]*"[^"]*"' | sed 's/.*"id"[[:space:]]*:[[:space:]]*"\([^"]*\)".*/\1/')
# Then try text format with id: prefix (common in older notarytool output)
elif echo "$SUBMISSION_OUTPUT" | grep -q 'id:'; then
    SUBMISSION_ID=$(echo "$SUBMISSION_OUTPUT" | grep -o 'id:[[:space:]]*[[:alnum:]]*' | sed 's/id:[[:space:]]*\([[:alnum:]]*\).*/\1/')
# Try to match "id" : "uuid-format" specifically
elif echo "$SUBMISSION_OUTPUT" | grep -Eq '"[[:alpha:]_]+id"[[:space:]]*:[[:space:]]*"[0-9a-f-]+"'; then
    SUBMISSION_ID=$(echo "$SUBMISSION_OUTPUT" | grep -o '"[[:alpha:]_]*id"[[:space:]]*:[[:space:]]*"[0-9a-f-]*"' | head -1 | sed 's/.*"[[:alpha:]_]*id"[[:space:]]*:[[:space:]]*"\([0-9a-f-]*\)".*/\1/')
# Finally try to find any UUID-like pattern as a fallback
else
    SUBMISSION_ID=$(echo "$SUBMISSION_OUTPUT" | grep -o '[0-9a-f]\{8\}-[0-9a-f]\{4\}-[0-9a-f]\{4\}-[0-9a-f]\{4\}-[0-9a-f]\{12\}')
fi

if [ -z "$SUBMISSION_ID" ]; then
    echo "Error: Failed to get submission ID from notarization output."
    echo "Raw output was: $SUBMISSION_OUTPUT"
    echo "Continuing with DMG creation despite notarization failure..."
    # Continue with DMG creation instead of exiting
    SKIP_NOTARIZATION=true
else
    SKIP_NOTARIZATION=false

    if [ "$SKIP_NOTARIZATION" != "true" ]; then
        echo "Notarization submission ID: $SUBMISSION_ID. Waiting for processing..."
        NOTARY_RESULT=$(xcrun notarytool wait --apple-id "$APPLE_ACCOUNT_EMAIL" \
            --team-id "$APPLE_TEAM_ID" \
            --password "$APPLE_APP_SPECIFIC_PASSWORD" \
            "$SUBMISSION_ID")
    fi
fi

echo "Notarization result: $NOTARY_RESULT"

# Check if notarization was successful - try multiple patterns to match success
if ! echo "$NOTARY_RESULT" | grep -qE "status:.*Accepted|Status:.*success|accepted|COMPLETE"; then
    echo "Notarization failed. Getting log for more details..."
    NOTARY_LOG=$(xcrun notarytool log --apple-id "$APPLE_ACCOUNT_EMAIL" \
        --team-id "$APPLE_TEAM_ID" \
        --password "$APPLE_APP_SPECIFIC_PASSWORD" \
        "$SUBMISSION_ID" 2>&1)
    
    echo "Notarization Log:"
    echo "$NOTARY_LOG"
    echo "Fix the issues before continuing."
    echo "Continuing with DMG creation despite notarization failure..."
    # Continue instead of exiting to allow DMG creation
else
    echo "Notarization successful!"
fi

# 9. Staple ticket for PKG
echo "Stapling notarization ticket to PKG..."
if [ "$SKIP_NOTARIZATION" = "true" ]; then
    echo "Skipping stapling since notarization was not performed successfully."
else
    xcrun stapler staple "$FINAL_INSTALLER_PATH" || { 
        echo "Stapler failed for PKG. This usually happens if notarization wasn't successful."
        echo "You may need to wait a bit longer for Apple's servers to process the notarization."
        echo "Try stapling manually later with: xcrun stapler staple $FINAL_INSTALLER_PATH"
    }
fi
echo "Proceeding to DMG creation..."

# 10. Create DMG installer
echo "Creating DMG installer..."

# Clean up previous DMG files
rm -rf "$DMG_STAGING_DIR"
rm -f "$TEMP_DMG_NAME"
rm -f "$FINAL_DMG_PATH"
mkdir -p "$DMG_STAGING_DIR"

# Create Documentation and Licenses folders
echo "Creating documentation structure for DMG..."
mkdir -p "$DMG_STAGING_DIR/Documentation"
mkdir -p "$DMG_STAGING_DIR/Licenses"

# Copy PKG installer to DMG staging with a simplified name
STANDARD_PKG_NAME="Eclipsa App.pkg"
cp "$FINAL_INSTALLER_PATH" "$DMG_STAGING_DIR/$STANDARD_PKG_NAME" || { echo "Failed to copy PKG to DMG staging"; exit 1; }
echo "Copied installer to DMG as $STANDARD_PKG_NAME"

# Copy license to Licenses folder
cp "$LICENSE_FILE" "$DMG_STAGING_DIR/Licenses/LICENSE.txt" || { echo "Failed to copy license to DMG"; exit 1; }

# Create a documentation file with plugin information
echo "Generating documentation file..."
cat > "$DMG_STAGING_DIR/Documentation/Documentation.txt" << EOF
# Eclipsa Audio Plugins

The Eclipsa Audio Renderer and Audio Element plugins provide music and audio creators with
tools for working with immersive audio formats.

## Installed Plugins
EOF

# Add plugin-specific information to documentation
case "$PLUGIN_FORMAT" in
    aax)
        cat >> "$DMG_STAGING_DIR/Documentation/Documentation.txt" << EOF
This package contains AAX plugins compatible with Pro Tools.

## Installation Locations
• AAX plugins: /Library/Application Support/Avid/Audio/Plug-Ins
EOF
        ;;
    vst3)
        cat >> "$DMG_STAGING_DIR/Documentation/Documentation.txt" << EOF
This package contains VST3 plugins compatible with VST3-supporting DAWs.

## Installation Locations
• VST3 plugins: /Library/Audio/Plug-Ins/VST3
EOF
        ;;
    au)
    cat >> "$DMG_STAGING_DIR/Documentation/Documentation.txt" << EOF
This package contains AU plugins compatible with AU-supporting DAWs.

## Installation Locations
• AU plugins: /Library/Audio/Plug-Ins/Components
EOF
esac

# Process background image if available - with optimal sizing for the DMG window
if [ -f "$DMG_BACKGROUND_FILE" ]; then
    echo "Setting up DMG background image with optimal sizing..."
    mkdir -p "$DMG_STAGING_DIR/.background"
    
    echo "Using sips for background processing (max dimension 640px)..."
    sips -Z 640 "$DMG_BACKGROUND_FILE" --out "$RESIZED_BACKGROUND" || {
        echo "Warning: sips processing failed for DMG background. Copying original."
        cp "$DMG_BACKGROUND_FILE" "$RESIZED_BACKGROUND"
    }
    
    # Copy the processed background to the DMG staging area
    cp "$RESIZED_BACKGROUND" "$DMG_STAGING_DIR/.background/background.png"
    echo "DMG background image prepared."
fi

# Make sure volume is unmounted first
echo "Checking for any existing DMG mounts..."
if [ -d "$MOUNT_POINT" ]; then
    echo "Found existing mount at $MOUNT_POINT - attempting to unmount..."
    hdiutil detach "$MOUNT_POINT" -force || echo "Warning: Could not unmount existing volume (may not have been mounted)"
fi

# Create temporary DMG
echo "Creating temporary DMG..."
hdiutil create -volname "$DMG_TITLE" -srcfolder "$DMG_STAGING_DIR" -ov -format UDRW "$TEMP_DMG_NAME" || { echo "Failed to create temp DMG"; exit 1; }

# Mount the DMG for customization
echo "Mounting DMG for customization..."
hdiutil attach -readwrite -noverify -noautoopen "$TEMP_DMG_NAME" || { echo "Failed to mount DMG"; exit 1; }
sleep 2

if [ -d "$MOUNT_POINT" ]; then
    # Set custom icon for the DMG volume using multiple techniques for reliability
    if [ -f "$CUSTOM_ICON_FILE" ]; then
        echo "Applying custom icon to DMG volume using comprehensive approach..."
        
        # Create high-quality iconset with all required sizes
        mkdir -p "$TMP_ICONSET"
        
        # Generate multiple icon sizes for best results (macOS requires various sizes)
        for size in 16 32 64 128 256 512 1024; do
            echo "Creating ${size}x${size} icon..."
            sips -z $size $size "$CUSTOM_ICON_FILE" --out "$TMP_ICONSET/icon_${size}x${size}.png" > /dev/null 2>&1
            
            # Add retina versions for all but the largest size
            if [ $size -lt 1024 ]; then
                doubleSize=$((size * 2))
                sips -z $doubleSize $doubleSize "$CUSTOM_ICON_FILE" --out "$TMP_ICONSET/icon_${size}x${size}@2x.png" > /dev/null 2>&1
            fi
        done
        
        # Convert iconset to icns - try multiple methods for reliability
        DMG_ICON_PATH="./custom_dmg_icon.icns"
        iconutil -c icns "$TMP_ICONSET" -o "$DMG_ICON_PATH" || { 
            echo "Warning: iconutil failed, trying alternative method with sips..."
            sips -s format icns "$CUSTOM_ICON_FILE" --out "$DMG_ICON_PATH" 2>/dev/null || {
                echo "Warning: All icon conversion methods failed, continuing without custom icon"
                DMG_ICON_PATH=""
            }
        }
        
        if [ -f "$DMG_ICON_PATH" ]; then
            echo "Custom icon successfully created, applying to DMG volume..."
            
            # Method 1: Standard .VolumeIcon.icns approach
            cp "$DMG_ICON_PATH" "$MOUNT_POINT/.VolumeIcon.icns"
            
            # Method 2: Set custom icon bit using xattr
            xattr -wx com.apple.FinderInfo "0000000000000000040000000000000000000000000000000000000000000000" "$MOUNT_POINT" 2>/dev/null || 
            echo "Warning: xattr failed, trying alternate method"
            
            # Method 3: Try fileicon if available for extra reliability
            if command -v fileicon >/dev/null 2>&1; then
                echo "Using fileicon to apply DMG volume icon as well..."
                fileicon set "$MOUNT_POINT" "$DMG_ICON_PATH" 2>/dev/null || 
                echo "Warning: fileicon for volume icon failed, but primary method may have succeeded"
            else
                echo "fileicon not found for DMG icon, relying on primary methods"
            fi
            
            # Touch the mount point to refresh Finder
            touch "$MOUNT_POINT"
            
            echo "DMG volume icon applied using multiple methods for maximum compatibility."
        fi
        
        # Clean up temporary iconset
        rm -rf "$TMP_ICONSET"
    fi
    
    # Create and run AppleScript to set DMG appearance
    echo "Setting DMG appearance with refined layout..."
    
    # For self-hosted runners, we need to ensure AppleScript runs in the correct user context
    CURRENT_USER=$(whoami)
    echo "Running AppleScript as user: $CURRENT_USER"
    
    cat > ./set_dmg_appearance.applescript << EOF
tell application "Finder"
    -- Set a timeout to prevent hanging
    with timeout of 60 seconds
        try
            -- Activate Finder to ensure it's running
            activate
            delay 1
            
            tell disk "$DMG_TITLE"
                open
                
                -- Wait longer for window to be ready in automated environment
                delay 3
                
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
                set shows item info of theViewOptions to false
                set shows icon preview of theViewOptions to true
                
                -- Set background if available
                try
                    if exists file ".background:background.png" of container window then
                        set background picture of theViewOptions to file ".background:background.png"
                        delay 1
                    end if
                on error
                    -- Background setting failed, continue anyway
                end try
                
                -- Wait a moment for layout to settle
                delay 2
                
                -- Position elements (check existence first)
                try
                    if exists item "Eclipsa App.pkg" of container window then
                        set position of item "Eclipsa App.pkg" of container window to {350, 220}
                        delay 0.5
                    end if
                on error
                    log "Could not position Eclipsa App.pkg"
                end try
                
                try
                    if exists item "Documentation" of container window then
                        set position of item "Documentation" of container window to {250, 370}
                        delay 0.5
                    end if
                on error
                    log "Could not position Documentation"
                end try
                
                try
                    if exists item "Licenses" of container window then
                        set position of item "Licenses" of container window to {450, 370}
                        delay 0.5
                    end if
                on error
                    log "Could not position Licenses"
                end try
                
                -- Force update and save settings
                update without registering applications
                delay 2
                
                -- Close the window to save settings
                close
                delay 1
                
            end tell
        on error errMsg number errNum
            log "AppleScript error " & errNum & ": " & errMsg
            return errMsg
        end try
    end timeout
end tell
EOF

    # Ensure we have the correct permissions and run as the logged-in user
    if [[ "$CURRENT_USER" != "root" ]]; then
        # Run directly if not root
        echo "Running AppleScript directly as $CURRENT_USER..."
        osascript ./set_dmg_appearance.applescript || echo "Warning: AppleScript failed, but continuing with DMG creation..."
    else
        # If running as root, try to run as the console user
        CONSOLE_USER=$(stat -f%Su /dev/console 2>/dev/null || echo "")
        if [[ -n "$CONSOLE_USER" && "$CONSOLE_USER" != "root" ]]; then
            echo "Running AppleScript as console user: $CONSOLE_USER..."
            sudo -u "$CONSOLE_USER" osascript ./set_dmg_appearance.applescript || echo "Warning: AppleScript failed, but continuing with DMG creation..."
        else
            echo "Warning: No console user found, trying to run AppleScript as root..."
            osascript ./set_dmg_appearance.applescript || echo "Warning: AppleScript failed, but continuing with DMG creation..."
        fi
    fi
    
    rm -f ./set_dmg_appearance.applescript
    
    # Detach the DMG
    echo "Detaching DMG..."
    hdiutil detach "$MOUNT_POINT" -force || echo "Warning: detach failed but continuing"
    sleep 5
else
    echo "Warning: Failed to access mount point at $MOUNT_POINT - appearance customization skipped"
fi

# Convert to final compressed DMG
echo "Creating final compressed DMG..."
hdiutil convert "$TEMP_DMG_NAME" -format UDZO -imagekey zlib-level=9 -o "$FINAL_DMG_PATH" || { echo "Failed to create final DMG"; exit 1; }

# Clean up temporary DMG
rm -f "$TEMP_DMG_NAME"
rm -f "./custom_dmg_icon.icns"

# Notarize DMG
if [ -f "$FINAL_DMG_PATH" ]; then
    echo "Submitting DMG for notarization..."
    DMG_SUBMISSION_OUTPUT=$(xcrun notarytool submit "$FINAL_DMG_PATH" \
        --apple-id "$APPLE_ACCOUNT_EMAIL" \
        --team-id "$APPLE_TEAM_ID" \
        --password "$APPLE_APP_SPECIFIC_PASSWORD" \
        --output-format json 2>&1) || {
        echo "Error: notarytool submit command for DMG failed. Output: $DMG_SUBMISSION_OUTPUT"
        echo "Continuing without DMG notarization..."
    }
    
    # Print the output for debugging
    echo "DMG notarization submission output:"
    echo "$DMG_SUBMISSION_OUTPUT"
    
    # Try multiple ways to extract the ID
    # First try JSON pattern that looks like "id": "value"
    if echo "$DMG_SUBMISSION_OUTPUT" | grep -q '"id"'; then
        DMG_SUBMISSION_ID=$(echo "$DMG_SUBMISSION_OUTPUT" | grep -o '"id"[[:space:]]*:[[:space:]]*"[^"]*"' | sed 's/.*"id"[[:space:]]*:[[:space:]]*"\([^"]*\)".*/\1/')
    # Then try JSON pattern that looks like "id" : "value" (with space around colon)
    elif echo "$DMG_SUBMISSION_OUTPUT" | grep -q '"id"[[:space:]]*:[[:space:]]*"'; then
        DMG_SUBMISSION_ID=$(echo "$DMG_SUBMISSION_OUTPUT" | grep -o '"id"[[:space:]]*:[[:space:]]*"[^"]*"' | sed 's/.*"id"[[:space:]]*:[[:space:]]*"\([^"]*\)".*/\1/')
    # Then try text format with id: prefix (common in older notarytool output)
    elif echo "$DMG_SUBMISSION_OUTPUT" | grep -q 'id:'; then
        DMG_SUBMISSION_ID=$(echo "$DMG_SUBMISSION_OUTPUT" | grep -o 'id:[[:space:]]*[[:alnum:]]*' | sed 's/id:[[:space:]]*\([[:alnum:]]*\).*/\1/')
    # Try to match "id" : "uuid-format" specifically
    elif echo "$DMG_SUBMISSION_OUTPUT" | grep -Eq '"[[:alpha:]_]+id"[[:space:]]*:[[:space:]]*"[0-9a-f-]+"'; then
        DMG_SUBMISSION_ID=$(echo "$DMG_SUBMISSION_OUTPUT" | grep -o '"[[:alpha:]_]*id"[[:space:]]*:[[:space:]]*"[0-9a-f-]*"' | head -1 | sed 's/.*"[[:alpha:]_]*id"[[:space:]]*:[[:space:]]*"\([0-9a-f-]*\)".*/\1/')
    # Finally try to find any UUID-like pattern as a fallback
    else
        DMG_SUBMISSION_ID=$(echo "$DMG_SUBMISSION_OUTPUT" | grep -o '[0-9a-f]\{8\}-[0-9a-f]\{4\}-[0-9a-f]\{4\}-[0-9a-f]\{4\}-[0-9a-f]\{12\}')
    fi
    
    if [ -z "$DMG_SUBMISSION_ID" ]; then
        echo "Error: Failed to get submission ID from DMG notarization output."
        echo "Raw output was: $DMG_SUBMISSION_OUTPUT"
        echo "Continuing without notarizing DMG..."
    else
        echo "DMG notarization submission ID: $DMG_SUBMISSION_ID. Waiting for processing..."
        DMG_NOTARY_RESULT=$(xcrun notarytool wait --apple-id "$APPLE_ACCOUNT_EMAIL" \
            --team-id "$APPLE_TEAM_ID" \
            --password "$APPLE_APP_SPECIFIC_PASSWORD" \
            "$DMG_SUBMISSION_ID")

        echo "DMG notarization result: $DMG_NOTARY_RESULT"

        # Check if notarization was successful
        if ! echo "$DMG_NOTARY_RESULT" | grep -qE "status:.*Accepted|Status:.*success|accepted|COMPLETE"; then
            echo "DMG notarization failed. Getting log for more details..."
            xcrun notarytool log --apple-id "$APPLE_ACCOUNT_EMAIL" \
                --team-id "$APPLE_TEAM_ID" \
                --password "$APPLE_APP_SPECIFIC_PASSWORD" \
                "$DMG_SUBMISSION_ID"
            echo "DMG not notarized, but continuing..."
        else
            echo "DMG notarization successful!"
            
            # Staple notarization ticket to DMG
            echo "Stapling notarization ticket to DMG..."
            xcrun stapler staple "$FINAL_DMG_PATH" || { 
                echo "Stapler failed for DMG. This is non-critical, continuing..." 
            }
            echo "DMG processing complete."
        fi
    fi
else
    echo "Error: Final DMG not found at $FINAL_DMG_PATH"
    exit 1
fi

# 11. Clean up
echo "Cleaning up staging directories..."

# Ensure any mounted DMG is properly unmounted
if [ -d "$MOUNT_POINT" ]; then
    echo "Unmounting any remaining DMG at $MOUNT_POINT..."
    hdiutil detach "$MOUNT_POINT" -force || echo "Warning: Could not unmount DMG volume (may not have been mounted)"
    sleep 2
fi

rm -rf "$PACKAGING_STAGING_DIR"
rm -rf "$COMPONENT_PKG_DIR"
rm -rf "$DMG_STAGING_DIR"
rm -f "$RESIZED_BACKGROUND"
rm -rf "$TMP_ICONSET" 2>/dev/null || true
rm -f "./custom_dmg_icon.icns" 2>/dev/null || true

# Create a separate directory for distribution artifacts that contains only DMG
# Create a clean distribution directory that will contain ONLY the DMG file
DIST_DIR="build/dist"
echo "Creating clean distribution directory for final artifacts..."
mkdir -p "$DIST_DIR"

# Only copy the DMG file to the distribution directory (explicitly excluding the PKG)
echo "Copying DMG to distribution directory (excluding raw PKG)..."
cp "$FINAL_DMG_PATH" "$DIST_DIR/" || { echo "Failed to copy DMG to distribution directory"; exit 1; }

# Verify the DMG was copied successfully
if [ -f "$DIST_DIR/$(basename "$FINAL_DMG_PATH")" ]; then
    echo "Successfully copied DMG to distribution directory: $DIST_DIR/$(basename "$FINAL_DMG_PATH")"
else
    echo "Warning: DMG copy verification failed"
fi

echo "-------------------------------------------------------"
echo "SUCCESS: Build and packaging completed!"
echo "PKG installer: $FINAL_INSTALLER_PATH"
echo "DMG installer: $FINAL_DMG_PATH"
echo "Distribution artifact: $DIST_DIR/$(basename "$FINAL_DMG_PATH")"
echo "Plugin format: $PLUGIN_FORMAT"
if [ "$SKIP_PACE_WRAPPING" = "true" ]; then echo "(PACE wrapping was SKIPPED)"; else echo "(PACE wrapping was INCLUDED)"; fi
echo "-------------------------------------------------------"

# Remove the raw PKG file to ensure only the DMG is distributed
# This is by design - we only want to distribute the DMG which contains the PKG
echo "Removing standalone PKG file to ensure consistent distribution..."
rm -f "$FINAL_INSTALLER_PATH"
echo "Note: Individual PKG installer has been removed to avoid confusion."
echo "The standardized DMG installer is the only artifact that should be distributed."

exit 0