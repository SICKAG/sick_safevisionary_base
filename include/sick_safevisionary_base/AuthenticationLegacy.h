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
#include "VisionaryControl.h"

namespace visionary {

class AuthenticationLegacy : public IAuthentication
{
public:
  explicit AuthenticationLegacy(VisionaryControl& vctrl);
  virtual ~AuthenticationLegacy();

  virtual bool login(UserLevel userLevel, const std::string& password);
  virtual bool logout();

private:
  VisionaryControl& m_VisionaryControl;
};

} // namespace visionary
