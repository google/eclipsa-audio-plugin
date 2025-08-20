#include <gtest/gtest.h>

#include "codec_config.pb.h"
#include "filesystem"
#include "google/protobuf/text_format.h"
#include "ia_sequence_header.pb.h"
#include "iamf/include/iamf_tools/encoder_main_lib.h"
#include "user_metadata.pb.h"

static void AddIaSequenceHeader(
    iamf_tools_cli_proto::UserMetadata& user_metadata) {
  google::protobuf::TextFormat::ParseFromString(
      R"pb(
        primary_profile: PROFILE_VERSION_SIMPLE
        additional_profile: PROFILE_VERSION_SIMPLE
      )pb",
      user_metadata.add_ia_sequence_header_metadata());
}

static void AddCodecConfig(iamf_tools_cli_proto::UserMetadata& user_metadata) {
  google::protobuf::TextFormat::ParseFromString(
      R"pb(
        codec_config_id: 200
        codec_config {
          codec_id: CODEC_ID_LPCM
          num_samples_per_frame: 64
          audio_roll_distance: 0
          decoder_config_lpcm {
            sample_format_flags: LPCM_LITTLE_ENDIAN
            sample_size: 16
            sample_rate: 48000
          }
        }
      )pb",
      user_metadata.add_codec_config_metadata());
}

TEST(test_iamf, iamf_protobuf_sanity) { iamf_tools_cli_proto::UserMetadata md; }

TEST(test_iamf, iamf_sanity_test) {
  iamf_tools_cli_proto::UserMetadata md;
  std::string wav_dir, out_dir, iamf_dir;
  wav_dir = out_dir = iamf_dir = "/tmp";
  auto result = iamf_tools::TestMain(md, wav_dir, iamf_dir);
}

TEST(test_iamf, iamf_sanity_test_filegen) {
  iamf_tools_cli_proto::UserMetadata md;
  AddIaSequenceHeader(md);
  md.mutable_test_vector_metadata()->set_partition_mix_gain_parameter_blocks(
      false);

  // Setting a filename prefix makes the function output a .iamf file.
  md.mutable_test_vector_metadata()->set_file_name_prefix("empty");

  std::string wav_dir, out_dir, iamf_dir;
  wav_dir = out_dir = iamf_dir = "/tmp/";

  auto result = iamf_tools::TestMain(md, wav_dir, iamf_dir);

  ASSERT_TRUE(std::filesystem::exists(iamf_dir + "empty.iamf"));
  std::filesystem::remove(iamf_dir + "empty.iamf");
}