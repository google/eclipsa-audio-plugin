- Compiled from: https://github.com/AOMediaCodec/iamf-tools.git
- Commit: https://github.com/AOMediaCodec/iamf-tools/commit/93471884b25e8a5bb7ef43c1330aa90b37a574b0
- Version: 1.0.0

### Update notes
- The commit hash on line 2 must be updated to reflect the commit used for compiling the .dylib file.
- Ensure the format of line 2 remains unchanged, as the commit hash is required for logging purposes.
### Compile notes
- Compiled for ARM OSX (Sonoma 14.4.1, clang 15.0.0).
### Steps taken to integrate encoder library:
1. In the `iamf-tools` repository, modified `iamf-tools/iamf/api/encoder/BUILD` by adding `load("@rules_cc//cc:cc_binary.bzl", "cc_binary")` and appending the existing cc_library declaration with:
`
cc_binary(
    name = "iamf_encoder_factory_shared",
    linkshared = 1,
    srcs = ["iamf_encoder_factory.cc"],
    deps = [
        ":iamf_encoder_factory",
    ],
    visibility = ["//visibility:public"],
)
`
2. Ran the Bazel build with `bazel build --copt="-g" --strip="never"  //iamf/api/encoder:iamf_encoder_factory_shared --macos_minimum_os=14 --spawn_strategy=standalone --cxxopt="-std=c++20"`.
3. Copied the resulting .dylib file, the headers under `iamf-tools/iamf/include/iamf_tools`, and the protocol buffers under `iamf-tools/iamf/cli/proto` to the eclipsa repository.
4. The .dylib file contains an internal reference to where it was built. This path must be updated for the library to be linkable/loadable on OSX. This path was updated via an OSX command `install_name_tool -id @rpath/third_party/iamftools/lib/libiamf_encoder_factory_shared.dylib libiamf_encoder_factory_shared.dylib` to use a relative path internally. *A CMake command setting the RPath to the repository root was also added.*
5. Added the `protobuf` library as a submodule to build the protoc compiler to generate files required by `iamf-tools`, and to provide other non-generated required headers.
### Steps taken to integrate the decoder library:
1. In the `iamf-tools` repository, modified `iamf-tools/iamf/api/decoder/BUILD` by adding `load("@rules_cc//cc:cc_binary.bzl", "cc_binary")` and appending the existing cc_library declarations with:
`
cc_binary(
    name = "iamf_decoder_factory_shared",
    srcs = [
        "iamf_decoder.cc",
        "iamf_decoder_factory.cc"
    ],
    linkshared = 1,
    deps = [
        ":iamf_decoder_factory",
    ],
    visibility = ["//visibility:public"],
)
2. Ran the Bazel build with `bazel build --copt="-g" --strip="never"  //iamf/api/encoder:iamf_decoder_factory_shared --macos_minimum_os=14 --spawn_strategy=standalone --cxxopt="-std=c++20"`.
3. Copied the resulting .dylib file to the eclipsa repository.
4. The .dylib file contains an internal reference to where it was built. This path must be updated for the library to be linkable/loadable on OSX. This path was updated via an OSX command `install_name_tool -id @rpath/third_party/iamftools/lib/libiamf_decoder_factory_shared.dylib libiamf_decoder_factory_shared.dylib` to use a relative path internally. *A CMake command setting the RPath to the repository root was also added.*
### Summary of steps taken to integrate library:
- In the `iamf-tools` repo: add a shared library target in the iamf-tools/iamf/api/encoder/BUILD and iamf-tools/iamf/api/decoder/BUILD files. Build the repo.
- Copy the shared library, headerfile for library entrypoint, and .proto files over.
- Update the internal path of the .dylib via the OSX command line utility.
- Add protobufs as a submodule, as its CMake API is necessary for generating proto files at buildtime (and we need a couple headers). 
