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

param([string]$Version = "")

$ErrorActionPreference = "Stop"

$IAMF_TOOLS_REPO = "https://github.com/AOMediaCodec/iamf-tools.git"
$SCRIPT_DIR = $PSScriptRoot
$ROOT_DIR = Split-Path $SCRIPT_DIR -Parent
# Use a system temp path to avoid antivirus locks on files within the project tree.
$TEMP_DIR = Join-Path $env:TEMP "iamf_tools_temp_$([System.Guid]::NewGuid().ToString('N').Substring(0,8))"
$DEST_LIB_DIR = Join-Path $ROOT_DIR "lib"
$DEST_PROTO_DIR = Join-Path $ROOT_DIR "iamf\cli\proto"
$DEST_INCLUDE_DIR = Join-Path $ROOT_DIR "iamf\include\iamf_tools"

function Remove-DirectoryWithRetry {
    param([string]$Path, [int]$MaxAttempts = 5, [int]$DelaySeconds = 3)
    for ($i = 1; $i -le $MaxAttempts; $i++) {
        try {
            Remove-Item $Path -Recurse -Force -ErrorAction Stop
            return
        } catch {
            if ($i -eq $MaxAttempts) { throw }
            Write-Host "Waiting for directory lock to release (attempt $i/$MaxAttempts)..."
            Start-Sleep -Seconds $DelaySeconds
        }
    }
}

function Get-Dumpbin {
    $cmd = Get-Command dumpbin.exe -ErrorAction SilentlyContinue
    if ($cmd) { return $cmd.Source }
    $vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
    $vsPath = & $vswhere -latest -property installationPath
    $found = Get-ChildItem "$vsPath\VC\Tools\MSVC" -Filter "dumpbin.exe" -Recurse |
        Where-Object { $_.FullName -match "Hostx64\\x64" } | Select-Object -First 1
    if (-not $found) { throw "dumpbin.exe not found. Run from a Visual Studio Developer Command Prompt." }
    return $found.FullName
}

function Get-ExportedSymbol {
    param([string]$LibPath, [string]$Pattern)
    $dumpbin = Get-Dumpbin
    $escapedPattern = [regex]::Escape($Pattern)
    $output = & $dumpbin /SYMBOLS $LibPath
    $line = $output | Where-Object { $_ -match "External" -and $_ -match $escapedPattern } | Select-Object -First 1
    if (-not $line) { throw "Could not find exported symbol matching '$Pattern' in $LibPath" }
    return ($line -replace '.*\| (\S+) \(.*', '$1').Trim()
}

# Use the provided version tag, or fetch the latest release from GitHub.
if ($Version -ne "") {
    $VERSION = $Version
} else {
    Write-Host "Fetching latest iamf-tools release..."
    $release = Invoke-RestMethod -Uri "https://api.github.com/repos/AOMediaCodec/iamf-tools/releases/latest"
    $VERSION = $release.tag_name
    Write-Host "Latest release: $VERSION"
}

# Clone the repo. For a tag or branch, use a fast shallow clone.
# For a raw commit hash, do a treeless clone then check out the commit.
Write-Host "Cloning iamf-tools $VERSION..."
# Use --quiet to suppress git's stderr progress output, which PS5.1 treats as NativeCommandError.
if ($VERSION -match '^[0-9a-f]{40}$') {
    git clone --quiet --filter=blob:none $IAMF_TOOLS_REPO $TEMP_DIR
    if ($LASTEXITCODE -ne 0) { throw "git clone failed with exit code $LASTEXITCODE" }
    git -C $TEMP_DIR checkout --quiet $VERSION
    if ($LASTEXITCODE -ne 0) { throw "git checkout failed with exit code $LASTEXITCODE" }
} else {
    git clone --quiet --branch $VERSION --depth 1 $IAMF_TOOLS_REPO $TEMP_DIR
    if ($LASTEXITCODE -ne 0) { throw "git clone failed with exit code $LASTEXITCODE" }
}
$COMMIT_HASH = git -C $TEMP_DIR rev-parse HEAD
if ($LASTEXITCODE -ne 0) { throw "git rev-parse failed with exit code $LASTEXITCODE" }

# Append shared library target to BUILD file.
Write-Host "Adding shared library target..."
$BUILD_ADDITION = @'

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
    win_def_file = "iamf_tools.def",
    visibility = ["//visibility:public"],
)
'@
Add-Content -Path (Join-Path $TEMP_DIR "iamf\include\iamf_tools\BUILD") -Value $BUILD_ADDITION

New-Item -ItemType Directory -Force -Path "$DEST_LIB_DIR\Windows\Debug" | Out-Null
New-Item -ItemType Directory -Force -Path "$DEST_LIB_DIR\Windows\Release" | Out-Null

# Detect export symbols once from the Debug (dbg) build. Debug and Release use the
# same source and same Abseil version, so mangled symbol names are identical across
# both configs. dumpbin /SYMBOLS is unreliable on optimized libs, so we always
# read from the debug factory libs.
Write-Host "Building Debug factory libs to detect export symbols..."
Push-Location $TEMP_DIR
try {
    bazelisk build `
        "--compilation_mode=dbg" `
        --cxxopt="/std:c++20" `
        //iamf/include/iamf_tools:iamf_encoder_factory `
        //iamf/include/iamf_tools:iamf_decoder_factory
} finally {
    Pop-Location
}

Write-Host "Detecting export symbols from Debug libs..."
$encoderLib = "$TEMP_DIR\bazel-out\x64_windows-dbg\bin\iamf\include\iamf_tools\iamf_encoder_factory.lib"
$decoderLib = "$TEMP_DIR\bazel-out\x64_windows-dbg\bin\iamf\include\iamf_tools\iamf_decoder_factory.lib"
$encoderSym = Get-ExportedSymbol $encoderLib `
    "?CreateFileGeneratingIamfEncoder@IamfEncoderFactory@api@iamf_tools@@"
$decoderSym = Get-ExportedSymbol $decoderLib `
    "?Create@IamfDecoderFactory@api@iamf_tools@@"

Write-Host "  Encoder: $encoderSym"
Write-Host "  Decoder: $decoderSym"

# Write the .def file once; it is reused for both Debug and Release builds.
$DEF_PATH = Join-Path $TEMP_DIR "iamf\include\iamf_tools\iamf_tools.def"
"LIBRARY iamf_tools`nEXPORTS`n$encoderSym`n$decoderSym" |
    Set-Content -Path $DEF_PATH -Encoding ASCII

foreach ($config in @("dbg", "opt")) {
    $isDebug = $config -eq "dbg"
    $configLabel = if ($isDebug) { "Debug" } else { "Release" }
    $destDir = "$DEST_LIB_DIR\Windows\$configLabel"

    Write-Host "Building $configLabel shared library..."
    Push-Location $TEMP_DIR
    try {
        # Use /Od for Release to avoid an MSVC optimizer crash in the
        # mix_presentation / obu_sequencer OBU finalization path.
        # /O1 was tried but didn't prevent the crash; /Od (no optimization) matches
        # Debug behavior and is confirmed to be crash-free.
        # Note: @arrayVar splatting is unreliable for native commands in PS 5.1 --
        # use explicit branches instead.
        if ($isDebug) {
            bazelisk build "--compilation_mode=$config" --cxxopt="/std:c++20" //iamf/include/iamf_tools:iamf_tools
        } else {
            bazelisk build "--compilation_mode=$config" --cxxopt="/std:c++20" "--copt=/Od" //iamf/include/iamf_tools:iamf_tools
        }
    } finally {
        Pop-Location
    }

    Write-Host "Copying $configLabel library files..."
    Copy-Item "$TEMP_DIR\bazel-bin\iamf\include\iamf_tools\iamf_tools.dll"    "$destDir\" -Force
    Copy-Item "$TEMP_DIR\bazel-bin\iamf\include\iamf_tools\iamf_tools.if.lib" "$destDir\" -Force
}

# Copy and patch proto files, preserving any existing CMakeLists.txt.
Write-Host "Copying and updating proto files..."
New-Item -ItemType Directory -Force -Path $DEST_PROTO_DIR | Out-Null

$CMAKELISTS_BACKUP = $null
if (Test-Path "$DEST_PROTO_DIR\CMakeLists.txt") {
    $CMAKELISTS_BACKUP = [System.IO.Path]::GetTempFileName()
    Copy-Item "$DEST_PROTO_DIR\CMakeLists.txt" $CMAKELISTS_BACKUP
}

$PROTO_TEMP_DIR = Join-Path ([System.IO.Path]::GetTempPath()) ([System.Guid]::NewGuid().ToString())
New-Item -ItemType Directory -Path $PROTO_TEMP_DIR | Out-Null
Copy-Item "$TEMP_DIR\iamf\cli\proto\*.proto" $PROTO_TEMP_DIR

foreach ($file in Get-ChildItem "$PROTO_TEMP_DIR\*.proto") {
    (Get-Content $file.FullName) -replace 'import "iamf/cli/proto/', 'import "' | Set-Content $file.FullName
}
Copy-Item "$PROTO_TEMP_DIR\*.proto" "$DEST_PROTO_DIR\" -Force
Remove-Item $PROTO_TEMP_DIR -Recurse -Force

if ($null -ne $CMAKELISTS_BACKUP) {
    Copy-Item $CMAKELISTS_BACKUP "$DEST_PROTO_DIR\CMakeLists.txt"
    Remove-Item $CMAKELISTS_BACKUP
}

# Copy header files.
Write-Host "Copying header files..."
New-Item -ItemType Directory -Force -Path $DEST_INCLUDE_DIR | Out-Null
Copy-Item "$TEMP_DIR\iamf\include\iamf_tools\*.h" "$DEST_INCLUDE_DIR\" -Force

# Update commit and version in README.
Write-Host "Updating README.md..."
$VERSION_NUM = $VERSION -replace '^v', ''
$content = Get-Content "$ROOT_DIR\README.md"
$content = $content -replace '^- Commit:.*', "- Commit: https://github.com/AOMediaCodec/iamf-tools/commit/$COMMIT_HASH"
$content = $content -replace '^- Version:.*', "- Version: $VERSION_NUM"
$content | Set-Content "$ROOT_DIR\README.md"

Write-Host "Cleaning up..."
Remove-DirectoryWithRetry $TEMP_DIR

Write-Host "Updated to $VERSION ($COMMIT_HASH) successfully!"
