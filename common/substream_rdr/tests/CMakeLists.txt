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

eclipsa_add_test(test_substream_rdr RendererFactory_test.cpp "substream_rdr;libear;juce::juce_audio_utils")
eclipsa_add_test(test_bed2bed_rdr BedToBedRdr_test.cpp "substream_rdr;libear;juce::juce_audio_utils")
eclipsa_add_test(test_hoa2bed_rdr HOAToBedRdr_test.cpp "substream_rdr;libear;juce::juce_audio_utils")
eclipsa_add_test(test_audio_panner AudioPanner_test.cpp "substream_rdr;juce::juce_audio_utils")
eclipsa_add_test(test_bin_rdr BinauralRdr_test.cpp "substream_rdr;libear")