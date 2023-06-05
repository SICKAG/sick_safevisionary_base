//
// Copyright note: Redistribution and use in source, with or without modification, are permitted.
// 
// Created: November 2019
// 
// SICK AG, Waldkirch
// email: TechSupport0905@sick.de

#if (_MSC_VER >= 1700)

#include <sstream>
#include <memory>

#include <string>
#include <random>
#include <chrono>

// TinyXML-2 XML DOM parser
#include "tinyxml2.h"

#include "VisionaryAutoIPScan.h"
#include "UdpSocket.h"

namespace visionary 
{

VisionaryAutoIPScan::VisionaryAutoIPScan()
{
}

VisionaryAutoIPScan::~VisionaryAutoIPScan()
{
}

std::vector<VisionaryAutoIPScan::DeviceInfo> VisionaryAutoIPScan::doScan(int timeOut, const std::string& broadcastAddress, uint16_t port)
{
  // Init Random generator
  std::random_device rd;
  std::default_random_engine mt(rd());
  unsigned int teleIdCounter = mt();
  std::vector<VisionaryAutoIPScan::DeviceInfo> deviceList;

  std::unique_ptr<UdpSocket> pTransport(new UdpSocket());

  if (pTransport->connect(broadcastAddress, htons(port)) != 0)
  {
    return deviceList;
  }

  // AutoIP Discover Packet
  std::vector<uint8_t> autoIpPacket;
  autoIpPacket.push_back(0x10); //CMD
  autoIpPacket.push_back(0x0); //reserved
  //length of datablock
  autoIpPacket.push_back(0x0);
  autoIpPacket.push_back(0x0);
  //Mac address
  autoIpPacket.push_back(0xFF);
  autoIpPacket.push_back(0xFF);
  autoIpPacket.push_back(0xFF);
  autoIpPacket.push_back(0xFF);
  autoIpPacket.push_back(0xFF);
  autoIpPacket.push_back(0xFF);
  // telgram id
  autoIpPacket.push_back(0x0);
  autoIpPacket.push_back(0x0);
  autoIpPacket.push_back(0x0);
  autoIpPacket.push_back(0x0);
  //reserved
  autoIpPacket.push_back(0x0);
  autoIpPacket.push_back(0x0);

  // Replace telegram id in packet
  unsigned int curtelegramID = teleIdCounter++;
  memcpy(&autoIpPacket.data()[10], &curtelegramID, 4u);

  // Send Packet
  pTransport->send(autoIpPacket);

  // Check for answers to Discover Packet
  const std::chrono::steady_clock::time_point startTime(std::chrono::steady_clock::now());
  while (true)
  {
    std::vector<std::uint8_t> receiveBuffer;
    const std::chrono::steady_clock::time_point now(std::chrono::steady_clock::now());
    if ((now - startTime) > std::chrono::milliseconds(timeOut))
    {
      break;
    }
    if (pTransport->recv(receiveBuffer, 1400) > 16) // 16 bytes minsize
    {
      unsigned int pos = 0;
      if (receiveBuffer[pos++] != 0x90) //0x90 = answer package id and 16 bytes minsize
      {
        continue;
      }
      pos += 1; // unused byte
      unsigned int payLoadSize = receiveBuffer[pos] << 8 | receiveBuffer[pos + 1];
      pos += 2;
      pos += 6; //Skip mac address(part of xml)
      unsigned int recvTelegramID = receiveBuffer[pos] | receiveBuffer[pos + 1] << 8 | receiveBuffer[pos + 2] << 16 | receiveBuffer[pos + 3] << 24;
      pos += 4;
      // check if it is a response to our scan
      if (recvTelegramID != curtelegramID)
      {
        continue;
      }
      pos += 2; // unused
      // Get XML Payload
      char xmlPayload[1400];
      memset(xmlPayload, 0, sizeof(xmlPayload));
      memcpy(&xmlPayload, &receiveBuffer[pos], payLoadSize);
      std::stringstream stringStream(xmlPayload);
      try
      {
        DeviceInfo dI = parseAutoIPXml(stringStream);
        deviceList.push_back(dI);
      }
      catch (...)
      {

      }
    }
  }
  return deviceList;
}

VisionaryAutoIPScan::DeviceInfo VisionaryAutoIPScan::parseAutoIPXml(std::stringstream& rStringStream)
{
  // Parse XML string into DOM
  tinyxml2::XMLDocument tree;
  auto tXMLError = tree.Parse(rStringStream.str());
  if (tXMLError != tinyxml2::XMLError::XML_SUCCESS) {
    std::printf("Reading XML tree in AutoIP NetScan result failed.");
    return false;
  }

  DeviceInfo dI;
  dI.DeviceName = "";
  dI.IpAddress  = "";
  dI.MacAddress = "";
  dI.Port       = "";
  dI.SubNet     = "";

  tinyxml2::XMLNode const* const ptDataSetsTree = tree.FirstChildElement("NetScanResult");
  if (ptDataSetsTree != 0)
  {
    // Query XML attributes
    tinyxml2::XMLAttribute const* ptAttr = 0;

    ptAttr = ptDataSetsTree->FindAttribute("DeviceType");
    if (ptAttr != 0)
    {
      dI.DeviceName = ptAttr->Value();
    }

    ptAttr = ptDataSetsTree->FindAttribute("IPAddress");
    if (ptAttr != 0)
    {
      dI.IpAddress = ptAttr->Value();
    }

    ptAttr = ptDataSetsTree->FindAttribute("MACAddr");
    if (ptAttr != 0)
    {
      dI.MacAddress = ptAttr->Value();
    }

    ptAttr = ptDataSetsTree->FindAttribute("HostPortNo");
    if (ptAttr != 0)
    {
      dI.Port = ptAttr->Value();
    }

    ptAttr = ptDataSetsTree->FindAttribute("IPMask");
    if (ptAttr != 0)
    {
      dI.subNet = ptAttr->Value();
    }
  }

  return dI;
}

}
#endif
