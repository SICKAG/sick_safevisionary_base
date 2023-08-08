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

#include "CoLaCommandType.h"
#include "CoLaError.h"
#include <cstdint>
#include <string>
#include <vector>

namespace visionary {

class CoLaCommand
{
private:
  std::vector<uint8_t> m_buffer;
  CoLaCommandType::Enum m_type;
  std::string m_name;
  size_t m_parameterOffset;
  CoLaError::Enum m_error;

  /// <summary>Construct a new <see cref="CoLaCommand" /> with the given command type, error, and
  /// name, but without any data.</summary>
  CoLaCommand(CoLaCommandType::Enum commandType, CoLaError::Enum error, const char* name);

public:
  /// <summary>Construct a new <see cref="CoLaCommand" /> from the given data buffer.</summary>
  CoLaCommand(std::vector<uint8_t> buffer);
  ~CoLaCommand();

  /// <summary>Get the binary data buffer.</summary>
  const std::vector<uint8_t>& getBuffer();

  /// <summary>Get the type of command.</summary>
  CoLaCommandType::Enum getType();

  /// <summary>Get the name of command.</summary>
  const char* getName();

  /// <summary>Get offset in bytes to where first parameter starts.</summary>
  size_t getParameterOffset();

  /// <summary>Get error.</summary>
  CoLaError::Enum getError();

  /// <summary>Create a command for network errors.</summary>
  static CoLaCommand networkErrorCommand();
};

} // namespace visionary
