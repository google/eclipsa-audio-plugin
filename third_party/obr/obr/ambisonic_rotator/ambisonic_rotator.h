/*
 * Copyright (c) 2025 Google LLC
 *
 * This source code is subject to the terms of the BSD 3-Clause Clear License,
 * which you can find in the LICENSE file, and the Open Binaural Renderer
 * Patent License 1.0, which you can find in the PATENTS file.
 */

#ifndef OBR_AMBISONIC_ROTATOR_AMBISONIC_ROTATOR_H_
#define OBR_AMBISONIC_ROTATOR_AMBISONIC_ROTATOR_H_

#include <vector>

#include "Eigen/Core"
#include "Eigen/Dense"
#include "obr/audio_buffer/audio_buffer.h"
#include "obr/common/misc_math.h"

// This code is forked from Resonance Audio's `hoa_rotator.h`.
namespace obr {

// Rotator for higher order Ambisonic sound fields. It supports ACN channel
// ordering and SN3D normalization (AmbiX).
class AmbisonicRotator {
 public:
  /*!\brief Constructs a sound field rotator of an arbitrary Ambisonic order.
   *
   * @param ambisonic_order Order of Ambisonic sound field.
   */
  explicit AmbisonicRotator(int ambisonic_order);

  /*!\brief Performs a smooth inplace rotation of a sound field buffer from
   * |current_rotation_| to |target_rotation|.
   *
   * @param target_rotation Target rotation to be applied to the input buffer.
   * @param input Ambisonic sound field input buffer to be rotated.
   * @param output Pointer to output buffer.
   * @return True if rotation has been applied.
   */
  bool Process(const WorldRotation& target_rotation, const AudioBuffer& input,
               AudioBuffer* output);

 private:
  /*!\brief Updates the rotation matrix with using supplied WorldRotation.
   *
   * @param rotation World rotation.
   */
  void UpdateRotationMatrix(const WorldRotation& rotation);

  // Order of the ambisonic sound field handled by the rotator.
  const int ambisonic_order_;

  // Current rotation which is used in the interpolation process in order to
  // compute new rotation matrix. Initialized with an identity rotation.
  WorldRotation current_rotation_;

  // Spherical harmonics rotation sub-matrices for each order.
  std::vector<Eigen::MatrixXf> rotation_matrices_;

  // Final spherical harmonics rotation matrix.
  Eigen::MatrixXf rotation_matrix_;
};

}  // namespace obr

#endif  // OBR_AMBISONIC_ROTATOR_AMBISONIC_ROTATOR_H_
