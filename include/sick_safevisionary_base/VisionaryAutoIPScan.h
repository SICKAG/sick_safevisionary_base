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
#include <vector>

namespace visionary {

static const uint16_t DEFAULT_PORT              = 30718;
static const std::string DEFAULT_BROADCAST_ADDR = "255.255.255.255";

class VisionaryAutoIPScan
{
public:
  struct DeviceInfo
  {
    std::string DeviceName;
    std::string MacAddress;
    std::string IpAddress;
    std::string SubNet;
    std::string Port;
  };
  VisionaryAutoIPScan();
  ~VisionaryAutoIPScan();

  /// <summary>
  /// Runs an autoIP scan and returns a list of devices
  /// </summary>
  /// <returns>A list of devices.</returns>
  std::vector<DeviceInfo> doScan(int timeOut,
                                 const std::string& broadcastAddress = DEFAULT_BROADCAST_ADDR,
                                 uint16_t port                       = DEFAULT_PORT);

private:
  DeviceInfo parseAutoIPXml(std::stringstream& rStringStream);
  static const short DEFAULT_PORT = 30718;
};

} // namespace visionary
