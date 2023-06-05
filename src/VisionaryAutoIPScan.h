//
// Copyright note: Redistribution and use in source, with or without modification, are permitted.
// 
// Created: May 2019
// 
// SICK AG, Waldkirch
// email: TechSupport0905@sick.de

#pragma once

#include <string>
#include <vector>

namespace visionary 
{

static const uint16_t DEFAULT_PORT = 30718;
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
  std::vector<DeviceInfo> doScan(int timeOut, const std::string& broadcastAddress = DEFAULT_BROADCAST_ADDR, uint16_t port = DEFAULT_PORT);

private:
  DeviceInfo parseAutoIPXml(std::stringstream& rStringStream);
  static const short DEFAULT_PORT = 30718;
};

}
