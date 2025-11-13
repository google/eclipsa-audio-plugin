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

eclipsa_add_test(test_fio_processor FileOutputProcessor_test.cpp "processors;iamf")
eclipsa_add_test(test_fio_processor FileOutputProcessor_PremierePro_test.cpp "processors;iamf")
eclipsa_add_test(test_processor_base ProcessorBase_test.cpp "processors;juce::juce_audio_utils")
eclipsa_add_test(test_render_processor Render_test.cpp "processors;juce::juce_audio_utils;iamf")
eclipsa_add_test(test_libear_sanity libear_test.cpp "libear")
eclipsa_add_test(test_gain_processor GainProcessor_test.cpp "processors")
eclipsa_add_test(test_ms_processor MSProcessor_test.cpp "processors")
eclipsa_add_test(test_channelmonitor_processor ChannelMonitorProcessor_test.cpp "processors")
eclipsa_add_test(test_panner_3dpanning Panner3DProcessor_Test.cpp "processors;juce_audio_utils")
eclipsa_add_test(test_remapping_processor RemappingProcessor_test.cpp "processors;juce_audio_utils")
eclipsa_add_test(test_audioelementplugin_routing AudioElementPluginRouting_test.cpp "processors;juce::juce_audio_utils")
eclipsa_add_test(test_loudness_proc LoudnessExportProcessor_test.cpp "processors")
eclipsa_add_test(test_ebu128_loudness MeasureEBU128_test.cpp "processors;lufs_meter")
eclipsa_add_test(test_mp4_iamf_demuxer MP4IAMFDemuxer_test.cpp "processors;iamf;iamfdec_utils")
eclipsa_add_test(test_iamf_writer IAMFFileWriter_test.cpp "processors;iamf")
eclipsa_add_test(test_iamf_reader IAMFFileReader_test.cpp "processors;iamf")