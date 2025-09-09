/*
 * Copyright (c) 2025 Google LLC
 *
 * This source code is subject to the terms of the BSD 3-Clause Clear License,
 * which you can find in the LICENSE file, and the Open Binaural Renderer
 * Patent License 1.0, which you can find in the PATENTS file.
 */

#ifndef OBR_RENDERER_OBR_IMPL_H_
#define OBR_RENDERER_OBR_IMPL_H_

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

#include "absl/log/check.h"
#include "absl/log/log.h"
#include "absl/status/status.h"
#include "absl/synchronization/mutex.h"
#include "obr/ambisonic_binaural_decoder/ambisonic_binaural_decoder.h"
#include "obr/ambisonic_binaural_decoder/fft_manager.h"
#include "obr/ambisonic_binaural_decoder/resampler.h"
#include "obr/ambisonic_encoder/ambisonic_encoder.h"
#include "obr/ambisonic_rotator/ambisonic_rotator.h"
#include "obr/audio_buffer/audio_buffer.h"
#include "obr/common/misc_math.h"
#include "obr/peak_limiter/peak_limiter.h"
#include "obr/renderer/audio_element_config.h"
#include "obr/renderer/audio_element_type.h"

namespace obr {

/*!\brief Implementation of the obr renderer.*/
class ObrImpl {
 public:
  /*!\brief Constructor.
   *
   * \param buffer_size_per_channel Buffer size per channel in samples.
   * \param sampling_rate Sampling rate in Hz.
   */
  ObrImpl(int buffer_size_per_channel, int sampling_rate);

  /*!\brief Destructor.
   */
  ~ObrImpl();

  /*!\brief Processes planar audio data.
   *
   * \param input_buffer Input buffer with planar audio data.
   * \param output_buffer Output buffer with planar audio data.
   */
  void Process(const AudioBuffer& input_buffer, AudioBuffer* output_buffer);

  /*!\brief Returns the buffer size per channel.
   *
   * \return Buffer size per channel.
   */
  int GetBufferSizePerChannel() const;

  /*!\brief Returns the sampling rate.
   *
   * \return Sampling rate.
   */
  int GetSamplingRate() const;

  /*!\brief Returns the number of input channels.
   *
   * \return Number of input channels.
   */
  size_t GetNumberOfInputChannels();

  /*!\brief Returns the number of output channels.
   *
   * \return Number of output channels.
   */
  size_t GetNumberOfOutputChannels();

  /*!\brief Returns the number of audio elements set within the renderer.
   *
   * \return Number of audio elements.
   */
  size_t GetNumberOfAudioElements();

  /*!\brief Adds an audio element to the renderer.
   * Creates an instance of AudioElementConfig, conducts all necessary checks,
   * populates with config data and updates renderer's DSP.
   * This method should handle all necessary DSP resource allocations.
   *
   * \param type Type of the audio element.
   * \param sub_type Subtype of the audio element.
   * \return `absl::OkStatus()` if successful. A specific status on failure.
   */
  absl::Status AddAudioElement(AudioElementType type);

  /*!\brief Removes the last added audio element from the renderer.
   * This method should handle all necessary DSP resource deallocations.
   *
   * \return `absl::OkStatus()` if successful. A specific status on failure.
   */
  absl::Status RemoveLastAudioElement();

  /*!\brief Sets the position of an audio object.
   *
   * \param audio_element_index Index of the audio element containing the
   * object.
   * \param azimuth Azimuth in degrees.
   * \param elevation Elevation in degrees.
   * \param distance Distance in meters.
   * \return `absl::OkStatus()` if successful. A specific status on failure.
   */
  absl::Status UpdateObjectPosition(size_t audio_element_index, float azimuth,
                                    float elevation, float distance);

  /*! \brief Enables or disables head tracking.
   *
   * \param enable_head_tracking True to enable head tracking, false to disable.
   */
  void EnableHeadTracking(bool enable_head_tracking);

  /*!\brief Sets the head rotation.
   * The head rotation expressed using quaternions is used to counter-rotate the
   * intermediate Ambisonic bed in order to produce stable sound sources in
   * binaural reproduction.
   * Use the following reference frame:
   * X - right
   * Y - up
   * Z - back (swap sign of z if using front facing Z coordinate)
   *
   * @param w
   * @param x
   * @param y
   * @param z
   * @return `absl::OkStatus()` if successful. A specific status on failure.
   */
  absl::Status SetHeadRotation(float w, float x, float y, float z);

  /*!\brief Returns a log message with the list of audio elements in a form of
   * an ASCII table.
   *
   * \return Log message with the list of audio elements.
   */
  std::string GetAudioElementConfigLogMessage();

 private:
  /*!\brief Resets the renderer's DSP to the initial state.
   *
   * \return `absl::OkStatus()` if successful. A specific status on failure.
   */
  absl::Status ResetDsp();

  /*!\brief Initializes the renderer's DSP.
   * First, it resets the DSP, then analyzes the list of requested audio
   * elements and initializes the necessary DSP resources.
   *
   * \return `absl::OkStatus()` if successful. A specific status on failure.
   */
  absl::Status InitializeDsp();

  /*!\brief Gets a vector of channel indices for the Ambisonic encoder.
   *
   * \return Vector of input channel indices.
   */
  std::vector<size_t> GetAmbisonicEncoderSourceChannelIndices();

  /*!\brief Updates the Ambisonic encoder.
   *
   * \return `absl::OkStatus()` if successful. A specific status on failure.
   */
  absl::Status UpdateAmbisonicEncoder();

  const int buffer_size_per_channel_;
  const int sampling_rate_;

  bool head_tracking_enabled_;
  WorldRotation world_rotation_;

  // Mutex to protect data accessed in different threads.
  mutable absl::Mutex mutex_;

  std::vector<AudioElementConfig> audio_elements_;
  AudioBuffer ambisonic_encoder_input_buffer_, ambisonic_mix_bed_;
  std::unique_ptr<AmbisonicEncoder> ambisonic_encoder_;
  Resampler resampler_;
  FftManager fft_manager_;
  std::unique_ptr<AudioBuffer> sh_hrirs_L_, sh_hrirs_R_;
  std::unique_ptr<AmbisonicRotator> ambisonic_rotator_;
  std::unique_ptr<AmbisonicBinauralDecoder> ambisonic_binaural_decoder_;
  std::unique_ptr<PeakLimiter> peak_limiter_;
};

}  // namespace obr

#endif  // OBR_RENDERER_OBR_IMPL_H_
