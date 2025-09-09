/*
 * Copyright (c) 2025 Google LLC
 *
 * This source code is subject to the terms of the BSD 3-Clause Clear License,
 * which you can find in the LICENSE file, and the Open Binaural Renderer
 * Patent License 1.0, which you can find in the PATENTS file.
 */

#ifndef OBR_COMMON_MISC_MATH_H_
#define OBR_COMMON_MISC_MATH_H_

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>

#include "Eigen/Core"
#include "Eigen/Dense"
#include "Eigen/Geometry"
#include "absl/log/check.h"
#include "constants.h"

namespace obr {

/*!\brief Finds the greatest common divisor between two integer values using the
 * Euclidean algorithm. Always returns a positive integer.
 *
 * \param a First of the two integer values.
 * \param b Second of the two integer values.
 * \return The greatest common divisor of the two integer values.
 */
inline int FindGcd(int a, int b) {
  a = std::abs(a);
  b = std::abs(b);
  int temp_value = 0;
  while (b != 0) {
    temp_value = b;
    b = a % b;
    a = temp_value;
  }
  return a;
}

/*!\brief Finds the next power of two from an integer. This method works with
 * values representable by unsigned 32 bit integers.
 *
 * \param input Integer value.
 * \return The next power of two from `input`.
 */
inline size_t NextPowTwo(size_t input) {
  // Ensure the value fits in a uint32_t.
  DCHECK_LT(static_cast<uint64_t>(input),
            static_cast<uint64_t>(std::numeric_limits<uint32_t>::max()));
  uint32_t number = static_cast<uint32_t>(--input);
  number |= number >> 1;   // Take care of 2 bit numbers.
  number |= number >> 2;   // Take care of 4 bit numbers.
  number |= number >> 4;   // Take care of 8 bit numbers.
  number |= number >> 8;   // Take care of 16 bit numbers.
  number |= number >> 16;  // Take care of 32 bit numbers.
  number++;
  return static_cast<size_t>(number);
}

/*!\brief Returns the factorial (!) of x. If x < 0, it returns 0.
 *
 * \param x Input to take factorial of.
 * \return Computed factorial of input; 0 if the input is negative.
 */
inline float Factorial(int x) {
  if (x < 0) {
    return 0.0f;
  }
  float result = 1.0f;
  for (; x > 0; --x) {
    result *= static_cast<float>(x);
  }
  return result;
}

/*!\brief Returns the double factorial (!!) of x.
 *
 * For odd x:  1 * 3 * 5 * ... * (x - 2) * x.
 * For even x: 2 * 4 * 6 * ... * (x - 2) * x.
 * If x < 0, it returns 0.
 *
 * \param x Input to take double factorial of.
 * \return Computed double factorial of input; 0 if the input is negative.
 */
inline float DoubleFactorial(int x) {
  if (x < 0) {
    return 0.0f;
  }
  float result = 1.0f;
  for (; x > 0; x -= 2) {
    result *= static_cast<float>(x);
  }
  return result;
}

/*!\brief Fast reciprocal of square-root.
 *
 * See: https://goo.gl/fqvstz for details.
 *
 * \param input The number to be inverse rooted.
 * \return An approximation of the reciprocal square root of `input`.
 */
inline float FastReciprocalSqrt(float input) {
  const float kThreeHalfs = 1.5f;
  const uint32_t kMagicNumber = 0x5f3759df;

  // Approximate a logarithm by aliasing to an integer.
  uint32_t integer = *reinterpret_cast<uint32_t*>(&input);
  integer = kMagicNumber - (integer >> 1);
  float approximation = *reinterpret_cast<float*>(&integer);
  const float half_input = input * 0.5f;
  // One iteration of Newton's method.
  return approximation *
         (kThreeHalfs - (half_input * approximation * approximation));
}

/*!\brief Computes `base`^`exp`, where `exp` is a *non-negative* integer.
 *
 * Computed using the squared exponentiation (a.k.a double-and-add) method.
 * When `T` is a floating point type, this has the same semantics as pow(), but
 * is much faster.
 * `T` can also be any integral type, in which case computations will be
 * performed in the value domain of this integral type, and overflow semantics
 * will be those of `T`.
 * You can also use any type for which `operator*=` is defined.

 * \param base Input to the exponent function. Any type for which *= is defined.
 * \param exp Integer exponent, must be greater than or equal to zero.
 * \return `base`^`exp`.
 */
template <typename T>
static inline T IntegerPow(T base, int exp) {
  DCHECK_GE(exp, 0);
  T result = static_cast<T>(1);
  while (true) {
    if (exp & 1) {
      result *= base;
    }
    exp >>= 1;
    if (!exp) break;
    base *= base;
  }
  return result;
}

class WorldRotation : public Eigen::Quaternion<float, Eigen::DontAlign> {
 public:
  // Inherits all constructors with 1-or-more arguments. Necessary because
  // MSVC12 doesn't support inheriting constructors.
  template <typename Arg1, typename... Args>
  WorldRotation(const Arg1& arg1, Args&&... args)
      : Quaternion(arg1, std::forward<Args>(args)...) {}

  // Constructs an identity rotation.
  WorldRotation() { this->setIdentity(); }

  // Returns the shortest arc between two |WorldRotation|s in radians.
  float AngularDifferenceRad(const WorldRotation& other) const {
    const Quaternion difference = this->inverse() * other;
    return static_cast<float>(Eigen::AngleAxisf(difference).angle());
  }
};

class WorldPosition : public Eigen::Matrix<float, 3, 1, Eigen::DontAlign> {
 public:
  // Inherits all constructors with 1-or-more arguments. Necessary because
  // MSVC12 doesn't support inheriting constructors.
  template <typename Arg1, typename... Args>
  WorldPosition(const Arg1& arg1, Args&&... args)
      : Matrix(arg1, std::forward<Args>(args)...) {}

  // Constructs a zero vector.
  WorldPosition();

  // Returns True if other |WorldPosition| differs by at least |kEpsilonFloat|.
  bool operator!=(const WorldPosition& other) const {
    return std::abs(this->x() - other.x()) > kEpsilonFloat ||
           std::abs(this->y() - other.y()) > kEpsilonFloat ||
           std::abs(this->z() - other.z()) > kEpsilonFloat;
  }
};

typedef Eigen::AngleAxis<float> AngleAxisf;

}  // namespace obr

#endif  // OBR_COMMON_MISC_MATH_H_
