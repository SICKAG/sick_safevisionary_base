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

namespace visionary {

// Available CoLa command types.
namespace CoLaCommandType {
enum Enum
{
  NETWORK_ERROR = -2,
  UNKNOWN       = -1,
  READ_VARIABLE,
  READ_VARIABLE_RESPONSE,
  WRITE_VARIABLE,
  WRITE_VARIABLE_RESPONSE,
  METHOD_INVOCATION,
  METHOD_RETURN_VALUE,
  COLA_ERROR
};
}

} // namespace visionary
