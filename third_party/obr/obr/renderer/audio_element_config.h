/*
 * Copyright (c) 2025 Google LLC
 *
 * This source code is subject to the terms of the BSD 3-Clause Clear License,
 * which you can find in the LICENSE file, and the Open Binaural Renderer
 * Patent License 1.0, which you can find in the PATENTS file.
 */

#ifndef OBR_AUDIO_ELEMENT_CONFIG_H_
#define OBR_AUDIO_ELEMENT_CONFIG_H_

#include <cstddef>
#include <vector>

#include "absl/log/check.h"
#include "absl/log/log.h"
#include "absl/status/statusor.h"
#include "absl/strings/string_view.h"
#include "obr/renderer/audio_element_type.h"
#include "obr/renderer/input_channel_config.h"

namespace obr {

/*!\brief Enumeration for selecting the binaural filter profile used to render
 * an Audio Element.
 */
enum class BinauralFilterProfile {
  kDirect = 0,
  kAmbient = 1,
  kReverberant = 2,
};

/*!\brief Configuration of an audio element. */
class AudioElementConfig {
 public:
  explicit AudioElementConfig(
      AudioElementType type,
      BinauralFilterProfile distance_type = BinauralFilterProfile::kAmbient);

  ~AudioElementConfig() = default;

  /*!\brief Returns the type of the audio element.
   *
   * \return Type of the audio element.
   */
  AudioElementType GetType() const;

  /*!\brief Returns the string representation of the audio element type.
   *
   * \return String representation of the audio element type.
   */
  absl::StatusOr<absl::string_view> GetTypeStr();

  /*!\brief Sets the first input channel index and updates the channel indices
   * the remaining input channels.
   *
   */
  void SetFirstChannelIndex(size_t first_channel);

  /*!\brief Returns the first input channel index.
   *
   * \return First input channel index.
   */
  size_t GetFirstChannelIndex() const;

  /*!\brief Returns the number of input channels.
   *
   * \return Number of input channels.
   */
  size_t GetNumberOfInputChannels() const;

  /*!\brief Returns the ambisonic input channels.
   *
   * \return Ambisonic channels.
   */
  std::vector<AmbisonicSceneInputChannel>& GetAmbisonicChannels();

  /*!\brief Returns the loudspeaker input channels.
   *
   * \return Loudspeaker channels.
   */
  std::vector<LoudspeakerLayoutInputChannel>& GetLoudspeakerChannels();

  /*!\brief Returns the object input channels.
   *
   * \return Object channels.
   */
  std::vector<AudioObjectInputChannel>& GetObjectChannels();

  /*!\brief Returns the Ambisonic order of the binaural filters to be used to
   * render this Audio Element.
   *
   * \return Ambisonic order of the binaural filters.
   */
  int GetBinauralFiltersAmbisonicOrder() const;

  /*!\brief Returns the binaural filter profile to be used to render this Audio
   * Element.
   *
   * \return Binaural filter profile.
   */
  BinauralFilterProfile GetBinauralFilterProfile() const {
    return binaural_filter_profile_;
  }

 private:
  AudioElementType type_;

  size_t first_channel_index_, number_of_input_channels_;

  std::vector<AmbisonicSceneInputChannel> ambisonic_channels_;
  std::vector<LoudspeakerLayoutInputChannel> loudspeaker_channels_;
  std::vector<AudioObjectInputChannel> object_channels_;

  int binaural_filters_ambisonic_order_;
  BinauralFilterProfile binaural_filter_profile_;
};

}  // namespace obr

#endif  // OBR_AUDIO_ELEMENT_CONFIG_H_
