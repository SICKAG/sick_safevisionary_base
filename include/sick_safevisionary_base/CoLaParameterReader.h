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

#include "CoLaCommand.h"
#include <cstdint>
#include <string>

namespace visionary {

/// <summary>
/// Class for reading data from a <see cref="CoLaCommand" />.
/// </summary>
class CoLaParameterReader
{
private:
  CoLaCommand m_command;
  size_t m_currentPosition;

public:
  CoLaParameterReader(CoLaCommand command);
  ~CoLaParameterReader();

  /// <summary>
  /// Rewind the position to the first parameter.
  /// </summary>
  void rewind();

  /// <summary>
  /// Read a signed short int (8 bit, range [-128, 127]) and advances position by 1 byte.
  /// </summary>
  int8_t readSInt();

  /// <summary>
  /// Read a unsigned short int (8 bit, range [0, 255]) and advances position by 1 byte.
  /// </summary>
  uint8_t readUSInt();

  /// <summary>
  /// Read a signed int (16 bit, range [-32768, 32767]) and advances position by 2 bytes.
  /// </summary>
  int16_t readInt();

  /// <summary>
  /// Read a unsigned int (16 bit, range [0, 65535]) and advances position by 2 bytes.
  /// </summary>
  uint16_t readUInt();

  /// <summary>
  /// Read a signed double int (32 bit) and advances position by 4 bytes.
  /// </summary>
  int32_t readDInt();

  /// <summary>
  /// Read a unsigned int (32 bit) and advances position by 4 bytes.
  /// </summary>
  uint32_t readUDInt();

  /// <summary>
  /// Read a IEEE-754 single precision (32 bit) and advances position by 4 bytes.
  /// </summary>
  float readReal();

  /// <summary>
  /// Read a IEEE-754 double precision (64 bit) and advances position by 8 bytes.
  /// </summary>
  double readLReal();

  /// <summary>
  /// Read a boolean and advance the position by 1 byte.
  /// </summary>
  bool readBool();

  /// <summary>
  /// Read a flex string, and advance position according to string size.
  /// </summary>
  std::string readFlexString();
};

} // namespace visionary
