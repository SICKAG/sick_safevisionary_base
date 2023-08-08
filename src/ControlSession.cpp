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

#include "sick_safevisionary_base/ControlSession.h"
#include "sick_safevisionary_base/CoLaParameterWriter.h"

namespace visionary {

ControlSession::ControlSession(IProtocolHandler& ProtocolHandler)
  : m_ProtocolHandler(ProtocolHandler)
{
}

ControlSession::~ControlSession() {}

CoLaCommand ControlSession::prepareRead(const std::string& varname)
{
  CoLaCommand cmd = CoLaParameterWriter(CoLaCommandType::READ_VARIABLE, varname.c_str()).build();
  return cmd;
}

CoLaCommand ControlSession::prepareWrite(const std::string& varname)
{
  CoLaCommand cmd = CoLaParameterWriter(CoLaCommandType::WRITE_VARIABLE, varname.c_str()).build();
  return cmd;
}

CoLaCommand ControlSession::prepareCall(const std::string& varname)
{
  CoLaCommand cmd =
    CoLaParameterWriter(CoLaCommandType::METHOD_INVOCATION, varname.c_str()).build();
  return cmd;
}

CoLaCommand ControlSession::send(const CoLaCommand& cmd)
{
  // ToDo: send command via CoLaProtocolHandler?
  // ProcolHandler needs to add e.g. header and checksum
  // Afterwards send to socket and receive the response.
  // return the response.
  return m_ProtocolHandler.send(cmd);
}

} // namespace visionary
