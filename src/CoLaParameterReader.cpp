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

#include "sick_safevisionary_base/CoLaParameterReader.h"
#include "sick_safevisionary_base/VisionaryEndian.h"

namespace visionary {

CoLaParameterReader::CoLaParameterReader(CoLaCommand command)
  : m_command(command)
{
  m_currentPosition = command.getParameterOffset();
}

CoLaParameterReader::~CoLaParameterReader() {}

void CoLaParameterReader::rewind()
{
  m_currentPosition = m_command.getParameterOffset();
}

int8_t CoLaParameterReader::readSInt()
{
  int8_t value = static_cast<int8_t>(m_command.getBuffer()[m_currentPosition]);
  m_currentPosition += 1;
  return value;
}

uint8_t CoLaParameterReader::readUSInt()
{
  const uint8_t value = m_command.getBuffer()[m_currentPosition];
  m_currentPosition += 1;
  return value;
}

int16_t CoLaParameterReader::readInt()
{
  const int16_t value =
    readUnalignColaByteOrder<int16_t>(&m_command.getBuffer()[m_currentPosition]);
  m_currentPosition += 2;
  return value;
}

uint16_t CoLaParameterReader::readUInt()
{
  const uint16_t value =
    readUnalignColaByteOrder<uint16_t>(&m_command.getBuffer()[m_currentPosition]);
  m_currentPosition += 2;
  return value;
}

int32_t CoLaParameterReader::readDInt()
{
  const int32_t value =
    readUnalignColaByteOrder<int32_t>(&m_command.getBuffer()[m_currentPosition]);
  m_currentPosition += 4;
  return value;
}

uint32_t CoLaParameterReader::readUDInt()
{
  const uint32_t value =
    readUnalignColaByteOrder<uint32_t>(&m_command.getBuffer()[m_currentPosition]);
  m_currentPosition += 4;
  return value;
}

float CoLaParameterReader::readReal()
{
  const float value = readUnalignColaByteOrder<float>(&m_command.getBuffer()[m_currentPosition]);
  m_currentPosition += 4;
  return value;
}

double CoLaParameterReader::readLReal()
{
  const double value = readUnalignColaByteOrder<double>(&m_command.getBuffer()[m_currentPosition]);
  m_currentPosition += 8;
  return value;
}

bool CoLaParameterReader::readBool()
{
  return readUSInt() == 1;
}

std::string CoLaParameterReader::readFlexString()
{
  std::string str("");
  uint16_t len = readUInt();
  if (len)
  {
    str =
      std::string(reinterpret_cast<const char*>(&m_command.getBuffer()[m_currentPosition]), len);
  }
  m_currentPosition += str.length();
  return str;
}
} // namespace visionary
