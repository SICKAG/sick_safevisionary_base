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
#include <string>

namespace visionary {

class IAuthentication
{
public:
  /// Available CoLa user levels.
  enum class UserLevel : int8_t
  {
    RUN               = 0,
    OPERATOR          = 1,
    MAINTENANCE       = 2,
    AUTHORIZED_CLIENT = 3,
    SERVICE           = 4
  };

  virtual ~IAuthentication(){};

  virtual bool login(UserLevel userLevel, const std::string& password) = 0;
  virtual bool logout()                                                = 0;
};

} // namespace visionary
