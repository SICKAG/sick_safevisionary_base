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
#include "IProtocolHandler.h"
#include "ITransport.h"

namespace visionary {

class CoLaBProtocolHandler : public IProtocolHandler
{
public:
  CoLaBProtocolHandler(ITransport& rTransport);
  ~CoLaBProtocolHandler();

  bool openSession(uint8_t sessionTimeout /*secs*/);
  void closeSession();

  // send cola cmd and receive cola response
  CoLaCommand send(CoLaCommand cmd);

private:
  ITransport& m_rTransport;
  uint8_t calculateChecksum(const std::vector<uint8_t>& buffer);
};

} // namespace visionary
