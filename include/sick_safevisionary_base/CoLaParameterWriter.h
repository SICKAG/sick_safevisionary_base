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
#include <vector>

#include "CoLaCommand.h"
#include "CoLaCommandType.h"

namespace visionary {

/// <summary>
/// Builder for constructing <see cref="CoLaCommand" />s.
/// </summary>
class CoLaParameterWriter
{
private:
  CoLaCommandType::Enum m_type;
  const char* m_name;
  std::vector<uint8_t> m_buffer;

public:
  /// <summary>
  /// Construct a new <see cref="CoLaParameterWriter" />.
  /// </summary>
  /// <param name="type">Type of command.</param>
  /// <param name="command">The command, e.g. for methods this should be the "communication name"
  /// from the CID.</param>
  CoLaParameterWriter(CoLaCommandType::Enum type, const char* name);

  ~CoLaParameterWriter();

  /// <summary>
  /// Add a signed short (8-bit, range [-128, 127]).
  /// </summary>
  /// <param name="sInt">SInt to add</param>
  /// <returns>This builder.</returns>
  CoLaParameterWriter& parameterSInt(const int8_t sInt);

  /// <summary>
  /// Add a unsigned short (8-bit, range [0, 255]).
  /// </summary>
  /// <param name="uSInt">USInt to add.</param>
  /// <returns>This builder.</returns>
  CoLaParameterWriter& parameterUSInt(const uint8_t uSInt);

  /// <summary>
  /// Add a signed int (16-bit).
  /// </summary>
  /// <param name="integer">Int to add.</param>
  /// <returns>This builder.</returns>
  CoLaParameterWriter& parameterInt(const int16_t integer);

  /// <summary>
  /// Add a unsigned int (16-bit, range [0, 65535]).
  /// </summary>
  /// <param name="uInt">UInt to add.</param>
  /// <returns>This builder.</returns>
  CoLaParameterWriter& parameterUInt(const uint16_t uInt);

  /// <summary>
  /// Add an signed double int (32-bit).
  /// </summary>
  /// <param name="dInt">dInt to add.</param>
  /// <returns>This builder.</returns>
  CoLaParameterWriter& parameterDInt(const int32_t dInt);

  /// <summary>
  /// Add an unsigned double int (32-bit, range [0, 4294967295]).
  /// </summary>
  /// <param name="uDInt">UDInt to add.</param>
  /// <returns>This builder.</returns>
  CoLaParameterWriter& parameterUDInt(const uint32_t uDInt);

  /// <summary>
  /// Add a IEEE-754 single precision (32-bit).
  /// </summary>
  /// <param name="real">Real to add.</param>
  /// <returns>This builder.</returns>
  CoLaParameterWriter& parameterReal(const float real);

  /// <summary>
  /// Add a IEEE-754 double precision (64-bit).
  /// </summary>
  /// <param name="lReal">Long real to add.</param>
  /// <returns>This builder.</returns>
  CoLaParameterWriter& parameterLReal(const double lReal);

  /// <summary>
  /// Add a boolean as a byte, with 0 representing false, and 1 representing true.
  /// </summary>
  /// <param name="boolean">Boolean to add.</param>
  /// <returns>This builder.</returns>
  CoLaParameterWriter& parameterBool(const bool boolean);

  /// <summary>
  /// Add the given password as a UDInt, using MD5 hashing.
  /// </summary>
  /// <param name="str">String to hash and add as parameter.</param>
  /// <returns>This builder.</returns>
  CoLaParameterWriter& parameterPasswordMD5(const std::string& str);

  /// <summary>
  /// Add a string as a flex string.
  /// </summary>
  /// <param name="string">String to add.</param>
  /// <returns>This builder.</returns>
  CoLaParameterWriter& parameterFlexString(const std::string& str);

  /// <summary>
  /// Add a string parameter, must be null-terminated.
  /// </summary>
  /// <param name="str">String to add.</param>
  /// <returns>This builder.</returns>
  CoLaParameterWriter& operator<<(const char* str);

  /// <summary>
  /// Add a signed short (8-bit, range [-128, 127]).
  /// </summary>
  /// <param name="sInt">SInt to add</param>
  /// <returns>This builder.</returns>
  CoLaParameterWriter& operator<<(const int8_t sInt);

  /// <summary>
  /// Add a unsigned short (8-bit, range [0, 255]).
  /// </summary>
  /// <param name="uSInt">USInt to add.</param>
  /// <returns>This builder.</returns>
  CoLaParameterWriter& operator<<(const uint8_t uSInt);

  /// <summary>
  /// Add a signed int (16-bit).
  /// </summary>
  /// <param name="integer">Int to add.</param>
  /// <returns>This builder.</returns>
  CoLaParameterWriter& operator<<(const int16_t integer);

  /// <summary>
  /// Add a unsigned int (16-bit, range [0, 65535]).
  /// </summary>
  /// <param name="uInt">UInt to add.</param>
  /// <returns>This builder.</returns>
  CoLaParameterWriter& operator<<(const uint16_t uInt);

  /// <summary>
  /// Add an signed double int (32-bit).
  /// </summary>
  /// <param name="dInt">dInt to add.</param>
  /// <returns>This builder.</returns>
  CoLaParameterWriter& operator<<(const int32_t dInt);

  /// <summary>
  /// Add an unsigned double int (32-bit, range [0, 4294967295]).
  /// </summary>
  /// <param name="uDInt">UDInt to add.</param>
  /// <returns>This builder.</returns>
  CoLaParameterWriter& operator<<(const uint32_t uDInt);

  /// <summary>
  /// Add a IEEE-754 single precision (32-bit).
  /// </summary>
  /// <param name="real">Real to add.</param>
  /// <returns>This builder.</returns>
  CoLaParameterWriter& operator<<(const float real);

  /// <summary>
  /// Add a IEEE-754 double precision (64-bit).
  /// </summary>
  /// <param name="lReal">Long real to add.</param>
  /// <returns>This builder.</returns>
  CoLaParameterWriter& operator<<(const double lReal);

  /// <summary>
  /// Add a boolean as a byte, with 0 representing false, and 1 representing true.
  /// </summary>
  /// <param name="boolean">Boolean to add.</param>
  /// <returns>This builder.</returns>
  CoLaParameterWriter& operator<<(const bool boolean);

  const CoLaCommand build();

private:
  void writeHeader(CoLaCommandType::Enum type, const char* name);

  /// <summary>
  /// Calculates the checksum for the given buffer.
  /// </summary>
  /// <param name="buffer">Buffer to calculate checksum for.</param>
  /// <returns>The calculated checksum.</returns>
  // static uint8_t calculateChecksum(const std::vector<uint8_t>& buffer);
};

} // namespace visionary
