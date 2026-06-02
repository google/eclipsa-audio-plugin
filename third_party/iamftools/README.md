- Compiled from: https://github.com/AOMediaCodec/iamf-tools.git
- Commit: https://github.com/AOMediaCodec/iamf-tools/commit/7542365c18d02ea4857c492963c50788cf20158e
- Version: 7542365c18d02ea4857c492963c50788cf20158e

### Update notes
- The commit hash on line 2 must be updated to reflect the commit used for compiling the .dylib file.
- Ensure the format of line 2 remains unchanged, as the commit hash is required for logging purposes.
- **Preferred method:** use `scripts/update.sh`. It handles all steps below automatically, including updating this README. Run with no arguments to use the latest release, or pass a version tag or commit hash: `./scripts/update.sh v1.2.0`

### Compile notes
- Compiled for ARM OSX (Sonoma 14.4.1, clang 15.0.0).

### Steps taken to integrate library (macOS -- automated by scripts/update.sh):
1. Cloned the iamf-tools repository. In the `iamf-tools` repository, appended the following shared library target to `iamf/include/iamf_tools/BUILD`:

```
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
```

2. Ran the Bazel build with `bazel build --copt="-g" --strip="never" --macos_minimum_os=14 --spawn_strategy=standalone --cxxopt="-std=c++20" //iamf/include/iamf_tools:iamf_tools`.
3. Copied the resulting .dylib from `bazel-bin/iamf/include/iamf_tools/libiamf_tools.dylib` to `third_party/iamftools/lib/`.
4. Copied the protobuf files from `iamf-tools/iamf/cli/proto` to `third_party/iamftools/iamf/cli/proto`.
5. Copied the headers from `iamf-tools/iamf/include/iamf_tools` to `third_party/iamftools/iamf/include/iamf_tools`.
6. Fixed the rpaths in the dylib:
    - Add third_party to the reference path for local builds: `install_name_tool -add_rpath @rpath/third_party/iamftools/lib/ libiamf_tools.dylib`
    - Add the dylib itself to the rpath: `install_name_tool -add_rpath @rpath/third_party/iamftools/lib/libiamf_tools.dylib libiamf_tools.dylib`
    - Change the build path id: `install_name_tool -id @rpath/third_party/iamftools/lib/libiamf_tools.dylib libiamf_tools.dylib`
7. Fixed the import paths in the proto files by replacing `import "iamf/cli/proto/name.proto"` with `import "name.proto"` for each proto file.
    - Note: Technical debt here -- we're unable to get the compiled proto files placed in `iamf/cli/proto` during the build unless there is a CMakeLists.txt in that directory, which then breaks the proto import paths. Worth revisiting when updating the library.
8. Updated the commit hash and version information at the top of this file (done automatically by `scripts/update.sh`).

### Windows - Run as administrator in Developer Command Prompt

1) Clone repository
2) Create C:\Dev\iamf-tools\iamf\include\iamf_tools\iamf_tools.def with:

      LIBRARY iamf_tools
      EXPORTS
      ?CreateFileGeneratingIamfEncoder@IamfEncoderFactory@api@iamf_tools@@SA?AV?$StatusOr@V?$unique_ptr@VIamfEncoderInterface@api@iamf_tools@@U?$default_delete@VIamfEncoderInterface@api@iamf_tools@@@std@@@std@@@lts_20250512@absl@@V?$basic_string_view@DU?$char_traits@D@std@@@std@@0@Z
      ?Create@IamfDecoderFactory@api@iamf_tools@@SA?AV?$unique_ptr@VIamfDecoderInterface@api@iamf_tools@@U?$default_delete@VIamfDecoderInterface@api@iamf_tools@@@std@@@std@@AEBUSettings@123@@Z

3) Open C:\Dev\iamf-tools\iamf\include\iamf_tools\BUILD
4) Add this at the end of the build file:

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

4) Run from root: bazel build --copt="/O3" //iamf/include/iamf_tools:iamf_tools --cxxopt="/std:c++20"
5) Copy the resulting iamf_tools.dll and iams_tools.if.lib to third_party\iamftools\lib\Windows\Release


