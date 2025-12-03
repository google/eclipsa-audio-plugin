- Compiled from: https://github.com/AOMediaCodec/iamf-tools.git
- Commit: https://github.com/AOMediaCodec/iamf-tools/commit/d7354b29d99c4d0198975568d4c23d5be0680f11
- Version: 2.0.0

### Update notes
- The commit hash on line 2 must be updated to reflect the commit used for compiling the .dylib file.
- Ensure the format of line 2 remains unchanged, as the commit hash is required for logging purposes.
### Compile notes
- Compiled for ARM OSX (Sonoma 14.4.1, clang 15.0.0).
### Steps taken to integrate library:
1. Cloned the iamf-toosl repository. In the `iamf-tools` repository, modified `iamf-tools/iamf/include/iamf_tools`. Added the following declaration for a shared library containing all the information from the other libraries:

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

2. Ran the Bazel build with `bazel build --copt="-g" --strip="never"  //iamf/include/iamf_tools:iamf_tools --macos_minimum_os=14 --spawn_strategy=standalone --cxxopt="-std=c++20"`.
3. Copied the resulting .dylib from `bazel-bin/iamf/include/iamf_tools/libiamf_tools.dylib` to `third_party/iamftools/lib/`.
4. Copied the protobuf files from 'iamftools/iamf/cli/proto' to 'third_party/iamftools/iamf/cli/proto'
5. Copied the headers from 'iamftools/iamf/include/iamf_tools' to 'third_party/iamftools/include/iamf_tools'
6. Fixed the rpaths in the dylib:
    - Add third_party to the reference path for local builds: "install_name_tool -add_rpath @rpath/third_party/iamftools/lib/  libiamf_tools.dylib"
    - Add the dylib itself to the rpath: "install_name_tool -add_rpath @rpath/third_party/iamftools/lib/libiamf_tools.dylib  libiamf_tools.dylib"
    - Finally, change the build path id: "install_name_tool -id @rpath/third_party/iamftools/lib/libiamf_tools.dylib libiamf_tools.dylib"
7. Fixed the import paths in the proto files by removing the full path and just using the proto files name.
    - For each proto file, on the lines "import iamf/cli/proto/name.proto" change to "import name.proto"
    - Note: Technical debt here, we're unable to get the compiled proto files to be placed in iamf/cli/proto during the build unless there is a CMAKE file in the iamf/cli/proto directory, which then breaks the proto import paths. There must be some way to get the proto files to be built from the top level CMAKE and placed in iamf/cli/proto on output, but currently unable to find one, seems like protobuf always puts the files in the same directory path the CMAKE file is in. Maybe worth revisting when updating the library.
8. Update the commit hash and version information at the top of this file, as it is used by CMAKE

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


