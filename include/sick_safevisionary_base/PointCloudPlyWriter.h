// -- BEGIN LICENSE BLOCK ----------------------------------------------
/*!
*  Copyright (C) 2023, SICK AG, Waldkirch, Germany
*  Copyright (C) 2023, FZI Forschungszentrum Informatik, Karlsruhe, Germany
*
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*    http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.

*/
// -- END LICENSE BLOCK ------------------------------------------------

#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "PointXYZ.h"

namespace visionary {

/// <summary>Class for writing point clouds to PLY files.</summary>
class PointCloudPlyWriter
{
public:
  /// <summary>Save a point cloud to a file in Polygon File Format (PLY), see:
  /// https://en.wikipedia.org/wiki/PLY_%28file_format%29 </summary> <param name="filename">The file
  /// to save the point cloud to</param> <param name="points">The points to save</param> <param
  /// name="useBinary">If the output file is binary or ascii</param> <returns>Returns true if write
  /// was successful and false otherwise</returns>
  static bool
  WriteFormatPLY(const char* filename, const std::vector<PointXYZ>& points, bool useBinary);

  /// <summary>Save a point cloud to a file in Polygon File Format (PLY) which has colors for each
  /// point, see: https://en.wikipedia.org/wiki/PLY_%28file_format%29 </summary> <param
  /// name="filename">The file to save the point cloud to</param> <param name="points">The points to
  /// save</param> <param name="rgbaMap">RGBA colors for each point, must be same length as
  /// points</param> <param name="useBinary">If the output file is binary or ascii</param>
  /// <returns>Returns true if write was successful and false otherwise</returns>
  static bool WriteFormatPLY(const char* filename,
                             const std::vector<PointXYZ>& points,
                             const std::vector<uint32_t>& rgbaMap,
                             bool useBinary);

  /// <summary>Save a point cloud to a file in Polygon File Format (PLY) which has intensities for
  /// each point, see: https://en.wikipedia.org/wiki/PLY_%28file_format%29 </summary> <param
  /// name="filename">The file to save the point cloud to</param> <param name="points">The points to
  /// save</param> <param name="intensityMap">Intensities for each point, must be same length as
  /// points</param> <param name="useBinary">If the output file is binary or ascii</param>
  /// <returns>Returns true if write was successful and false otherwise</returns>
  static bool WriteFormatPLY(const char* filename,
                             const std::vector<PointXYZ>& points,
                             const std::vector<uint16_t>& intensityMap,
                             bool useBinary);

  /// <summary>Save a point cloud to a file in Polygon File Format (PLY) which has intensities and
  /// colors for each point, see: https://en.wikipedia.org/wiki/PLY_%28file_format%29">Specification
  /// </summary> <param name="filename">The file to save the point cloud to</param> <param
  /// name="points">The points to save</param> <param name="rgbaMap">RGBA colors for each point,
  /// must be same length as points</param> <param name="intensityMap">Intensities for each point,
  /// must be same length as points</param> <param name="useBinary">If the output file is binary or
  /// ascii</param> <returns>Returns true if write was successful and false otherwise</returns>
  static bool WriteFormatPLY(const char* filename,
                             const std::vector<PointXYZ>& points,
                             const std::vector<uint32_t>& rgbaMap,
                             const std::vector<uint16_t>& intensityMap,
                             bool useBinary);

private:
  // No instantiations
  PointCloudPlyWriter();
  virtual ~PointCloudPlyWriter();
  const PointCloudPlyWriter& operator=(const PointCloudPlyWriter&);
  PointCloudPlyWriter(const PointCloudPlyWriter&);
};

} // namespace visionary
