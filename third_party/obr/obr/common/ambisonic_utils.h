/*
 * Copyright (c) 2025 Google LLC
 *
 * This source code is subject to the terms of the BSD 3-Clause Clear License,
 * which you can find in the LICENSE file, and the Open Binaural Renderer
 * Patent License 1.0, which you can find in the PATENTS file.
 */

#ifndef OBR_COMMON_AMBISONIC_UTILS_H_
#define OBR_COMMON_AMBISONIC_UTILS_H_

#include <cmath>
#include <cstddef>

#include "absl/log/check.h"
#include "obr/common/misc_math.h"

// This code is forked from Resonance Audio's `ambisonics/utils.h`.
namespace obr {

/*!\brief Computes ACN channel sequence from a degree and order.
 *
 * \param degree Degree of the spherical harmonic.
 * \param order Order of the spherical harmonic.
 * \return Computed ACN channel sequence.
 */
inline int AcnSequence(int degree, int order) {
  DCHECK_GE(degree, 0);
  DCHECK_LE(-degree, order);
  DCHECK_LE(order, degree);

  return degree * degree + degree + order;
}

/*!\brief Computes normalization factor for Schmidt semi-normalized harmonics.
 *
 * The Schmidt semi-normalized spherical harmonics is used in AmbiX.
 *
 * \param degree Degree of the spherical harmonic.
 * \param order Order of the spherical harmonic.
 * \return Computed normalization factor.
 */
inline float Sn3dNormalization(int degree, int order) {
  DCHECK_GE(degree, 0);
  DCHECK_LE(-degree, order);
  DCHECK_LE(order, degree);
  return std::sqrt((2.0f - ((order == 0) ? 1.0f : 0.0f)) *
                   Factorial(degree - std::abs(order)) /
                   Factorial(degree + std::abs(order)));
}

/*!\brief Returns the number of spherical harmonics for a periphonic ambisonic
 * sound field of |ambisonic_order|.
 */
inline size_t GetNumPeriphonicComponents(int ambisonic_order) {
  return static_cast<size_t>((ambisonic_order + 1) * (ambisonic_order + 1));
}

/*!\brief Returns the number of periphonic spherical harmonics (SHs) for a
 * particular Ambisonic order. E.g. number of 1st, 2nd or 3rd degree SHs in a
 * 3rd order sound field.
 */
inline size_t GetNumNthOrderPeriphonicComponents(int ambisonic_order) {
  if (ambisonic_order == 0) return 1;
  return static_cast<size_t>(GetNumPeriphonicComponents(ambisonic_order) -
                             GetNumPeriphonicComponents(ambisonic_order - 1));
}

/*!\brief Calculates the order of the current spherical harmonic channel as the
 * integer part of a square root of the channel number. Please note, that in
 * Ambisonics the terms 'order' (usually denoted as 'n') and 'degree' (usually
 * denoted as 'm') are used in the opposite meaning as in more traditional maths
 * or physics conventions: [1] C. Nachbar, F. Zotter, E. Deleflie, A. Sontacchi,
 * "AMBIX - A SUGGESTED AMBISONICS FORMAT", Proc. of the 2nd Ambisonics
 * Symposium, June 2-3 2011, Lexington, KY, https://goo.gl/jzt4Yy.
 */
inline int GetPeriphonicAmbisonicOrderForChannel(size_t channel) {
  return static_cast<int>(sqrtf(static_cast<float>(channel)));
}

/*!\brief Calculates the degree of the current spherical harmonic channel.
 *
 * Please note, that in Ambisonics the terms 'order' (usually denoted as 'n')
 * and 'degree' (usually denoted as 'm') are used in the opposite meaning as in
 * more traditional maths or physics conventions:
 * [1] C. Nachbar, F. Zotter, E. Deleflie, A. Sontacchi, "AMBIX - A SUGGESTED
 *     AMBISONICS FORMAT", Proc. of the 2nd Ambisonics Symposium, June 2-3 2011,
 *     Lexington, KY, https://goo.gl/jzt4Yy.
 */
inline int GetPeriphonicAmbisonicDegreeForChannel(size_t channel) {
  const int order = GetPeriphonicAmbisonicOrderForChannel(channel);
  return static_cast<int>(channel) - order * (order + 1);
}

/*!\brief Returns whether the given `num_channels` corresponds to a valid
 * ambisonic order configuration.
 */
inline bool IsValidAmbisonicOrder(size_t num_channels) {
  if (num_channels == 0) {
    return false;
  }
  // Number of channels must be a square number for valid ambisonic orders.
  const auto sqrt_num_channels = static_cast<size_t>(std::sqrt(num_channels));
  return num_channels == sqrt_num_channels * sqrt_num_channels;
}

}  // namespace obr

#endif  // OBR_COMMON_AMBISONIC_UTILS_H_
