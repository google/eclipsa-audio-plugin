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

eclipsa_add_test(test_audio_element AudioElement_test.cpp "data_structures")
eclipsa_add_test(test_file_export FileExport_test.cpp "data_structures")
eclipsa_add_test(test_room_setup RoomSetup_test.cpp "data_structures")
eclipsa_add_test(test_mix_presentation MixPresentation_test.cpp "data_structures")
eclipsa_add_test(test_channel_gains ChannelGains_test.cpp "data_structures")
eclipsa_add_test(test_mute_solo_type MSPlayback_test.cpp "data_structures")
eclipsa_add_test(test_audioElementSpatialLayout AudioElementSpatialLayout_test.cpp "data_structures")
eclipsa_add_test(test_active_mix_pres ActiveMixPresentation_test.cpp "data_structures")
eclipsa_add_test(test_mix_presentation_solo_mute MixPresentationSoloMute_test.cpp "data_structures")
eclipsa_add_test(test_mix_presentation_loudness MixPresentationLoudness_test.cpp "data_structures")