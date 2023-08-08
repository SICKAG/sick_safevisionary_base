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

#include "sick_safevisionary_base/AuthenticationLegacy.h"
#include "sick_safevisionary_base/CoLaParameterReader.h"
#include "sick_safevisionary_base/CoLaParameterWriter.h"

namespace visionary {

AuthenticationLegacy::AuthenticationLegacy(VisionaryControl& vctrl)
  : m_VisionaryControl(vctrl)
{
}

AuthenticationLegacy::~AuthenticationLegacy() {}

bool AuthenticationLegacy::login(UserLevel userLevel, const std::string& password)
{
  CoLaCommand loginCommand =
    CoLaParameterWriter(CoLaCommandType::METHOD_INVOCATION, "SetAccessMode")
      .parameterSInt(static_cast<int8_t>(userLevel))
      .parameterPasswordMD5(password)
      .build();
  CoLaCommand loginResponse = m_VisionaryControl.sendCommand(loginCommand);


  if (loginResponse.getError() == CoLaError::OK)
  {
    return CoLaParameterReader(loginResponse).readBool();
  }
  return false;
}

bool AuthenticationLegacy::logout()
{
  CoLaCommand runCommand  = CoLaParameterWriter(CoLaCommandType::METHOD_INVOCATION, "Run").build();
  CoLaCommand runResponse = m_VisionaryControl.sendCommand(runCommand);

  if (runResponse.getError() == CoLaError::OK)
  {
    return CoLaParameterReader(runResponse).readBool();
  }
  return false;
}

} // namespace visionary
