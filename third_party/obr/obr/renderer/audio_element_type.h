/*
 * Copyright (c) 2025 Google LLC
 *
 * This source code is subject to the terms of the BSD 3-Clause Clear License,
 * which you can find in the LICENSE file, and the Open Binaural Renderer
 * Patent License 1.0, which you can find in the PATENTS file.
 */

#ifndef OBR_AUDIO_ELEMENT_TYPE_H_
#define OBR_AUDIO_ELEMENT_TYPE_H_

#include <string>
#include <vector>

#include "absl/base/no_destructor.h"
#include "absl/container/btree_map.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/string_view.h"

namespace obr {

// TODO(b/400894670): Avoid using "AudioElement" in the class/function names.
/*!\brief Enums and inline functions used to configure the binaural renderer.*/
enum class AudioElementType : int {
  kInvalidType = 0,
  kAmbisonicsGroupBegin = 100,
  k1OA = 101,
  k2OA = 102,
  k3OA = 103,
  k4OA = 104,
  kAmbisonicsGroupEnd = 105,
  kLoudspeakerLayoutsGroupBegin = 200,
  kLayoutMono = 201,
  kLayoutStereo = 202,
  kLayout3_1_2_ch = 203,
  kLayout5_1_0_ch = 204,
  kLayout5_1_2_ch = 205,
  kLayout5_1_4_ch = 206,
  kLayout7_1_0_ch = 207,
  kLayout7_1_2_ch = 208,
  kLayout7_1_4_ch = 209,
  kLayout9_1_0_ch = 210,
  kLayout9_1_2_ch = 211,
  kLayout9_1_4_ch = 212,
  kLayout9_1_6_ch = 213,
  kLoudspeakerLayoutsGroupEnd = 214,
  kObjectsGroupBegin = 300,
  kObjectMono = 301,
  kObjectsGroupEnd = 302,
};

/*!\brief Ordered map containing the string representation of the audio element
 * types.
 */
// TODO(b/402659240): Unify the string representations used for logging and
//                    flag-parsing.
inline const absl::btree_map<AudioElementType, absl::string_view>&
GetAudioElementTypeStringMap() {
  static const absl::NoDestructor<
      absl::btree_map<AudioElementType, absl::string_view>>
      kAudioElementTypeStringMap([] {
        absl::btree_map<AudioElementType, absl::string_view> map = {
            {AudioElementType::k1OA, "k1OA"},
            {AudioElementType::k2OA, "k2OA"},
            {AudioElementType::k3OA, "k3OA"},
            {AudioElementType::k4OA, "k4OA"},
            {AudioElementType::kLayoutMono, "kLayoutMono"},
            {AudioElementType::kLayoutStereo, "kLayoutStereo"},
            {AudioElementType::kLayout3_1_2_ch, "kLayout3_1_2_ch"},
            {AudioElementType::kLayout5_1_0_ch, "kLayout5_1_0_ch"},
            {AudioElementType::kLayout5_1_2_ch, "kLayout5_1_2_ch"},
            {AudioElementType::kLayout5_1_4_ch, "kLayout5_1_4_ch"},
            {AudioElementType::kLayout7_1_0_ch, "kLayout7_1_0_ch"},
            {AudioElementType::kLayout7_1_2_ch, "kLayout7_1_2_ch"},
            {AudioElementType::kLayout7_1_4_ch, "kLayout7_1_4_ch"},
            {AudioElementType::kLayout9_1_0_ch, "kLayout9_1_0_ch"},
            {AudioElementType::kLayout9_1_2_ch, "kLayout9_1_2_ch"},
            {AudioElementType::kLayout9_1_4_ch, "kLayout9_1_4_ch"},
            {AudioElementType::kLayout9_1_6_ch, "kLayout9_1_6_ch"},
            {AudioElementType::kObjectMono, "kObjectMono"},
        };
        return map;
      }());

  return *kAudioElementTypeStringMap;
}

/*!\brief Returns the string representation of the audio element type.
 *
 * \param type Audio element type.
 * \return String representation of the audio element type.
 */
inline absl::StatusOr<absl::string_view> GetAudioElementTypeStr(
    AudioElementType type) {
  if (GetAudioElementTypeStringMap().contains(type)) {
    return GetAudioElementTypeStringMap().at(type);
  }
  return absl::InvalidArgumentError("Invalid audio element type.");
}

/*!\brief Returns the audio element type from its string representation.
 *
 * \param type_str String representation of the audio element type.
 * \return Audio element type.
 */
inline absl::StatusOr<AudioElementType> GetAudioElementTypeFromStr(
    absl::string_view type_str) {
  for (const auto& [type, str] : GetAudioElementTypeStringMap()) {
    if (str == type_str) {
      return type;
    }
  }
  return absl::InvalidArgumentError("Invalid audio element type string.");
}

/*!\brief Returns a vector of strings with Audio Element types accepted by
 * the renderer.
 *
 * \return Vector of strings with Audio Element types.
 */
inline std::vector<std::string> GetAvailableAudioElementTypesAsStr() {
  std::vector<std::string> types;
  for (const auto& [type, str] : GetAudioElementTypeStringMap()) {
    types.emplace_back(str);
  }
  return types;
}

/*!\brief Check if the audio element type is an Ambisonics type.
 *
 * \param type Audio element type.
 * \return True if the audio element type is an Ambisonics type.
 */
inline bool IsAmbisonicsType(AudioElementType type) {
  return type > AudioElementType::kAmbisonicsGroupBegin &&
         type < AudioElementType::kAmbisonicsGroupEnd;
}

/*!\brief Check if the audio element type is a loudspeaker layout type.
 *
 * \param type Audio element type.
 * \return True if the audio element type is a loudspeaker layout type.
 */
inline bool IsLoudspeakerLayoutType(AudioElementType type) {
  return type > AudioElementType::kLoudspeakerLayoutsGroupBegin &&
         type < AudioElementType::kLoudspeakerLayoutsGroupEnd;
}

/*!\brief Check if the audio element type is an object type.
 *
 * \param type Audio element type.
 * \return True if the audio element type is an object type.
 */
inline bool IsObjectType(AudioElementType type) {
  return type > AudioElementType::kObjectsGroupBegin &&
         type < AudioElementType::kObjectsGroupEnd;
}

/*!\brief Returns the Ambisonic order from the audio element type.
 *
 * \param type Audio element type.
 * \return Ambisonic order on success. A specific status on failure.
 */
inline absl::StatusOr<int> GetAmbisonicOrder(AudioElementType type) {
  switch (type) {
    case AudioElementType::k1OA:
      return 1;
    case AudioElementType::k2OA:
      return 2;
    case AudioElementType::k3OA:
      return 3;
    case AudioElementType::k4OA:
      return 4;
    default:
      return absl::InvalidArgumentError(
          "Invalid audio element type (not Ambisonics).");
  }
}

}  // namespace obr

#endif  // OBR_AUDIO_ELEMENT_TYPE_H_
