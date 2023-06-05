//
// Copyright note: Redistribution and use in source, with or without modification, are permitted.
// 
// Created: August 2020
// 
// SICK AG, Waldkirch
// email: TechSupport0905@sick.de

#pragma once

#include <string>
#include <vector>

#include "VisionaryData.h"

namespace visionary
{

class VisionaryTMiniData : public VisionaryData
{
public:
  VisionaryTMiniData();
  ~VisionaryTMiniData();

  //-----------------------------------------------
  // Getter Functions

    // Gets the radial distance map
  // The unit of the distancemap is 1/4 mm
  const std::vector<uint16_t>& getDistanceMap() const;

  // Gets the intensity map
  const std::vector<uint16_t>& getIntensityMap() const;

  // Gets the state map
  const std::vector<uint16_t>& getStateMap() const;

  // Calculate and return the Point Cloud in the camera perspective. Units are in meters.
  void generatePointCloud(std::vector<PointXYZ> &pointCloud);

  // factor to convert Radial distance map from fixed point to floating point
  static const float DISTANCE_MAP_UNIT;

protected:
  //-----------------------------------------------
  // functions for parsing received blob

  // Parse the XML Metadata part to get information about the sensor and the following image data.
  // Returns true when parsing was successful.
  bool parseXML(const std::string & xmlString, uint32_t changeCounter);

  // Parse the Binary data part to extract the image data.
  // some variables are commented out, because they are not used in this sample.
  // Returns true when parsing was successful.
  bool parseBinaryData(std::vector<uint8_t>::iterator itBuf, size_t length);

private:
  // Indicator for the received data sets
  DataSetsActive m_dataSetsActive;

  // Byte depth of images
  int m_distanceByteDepth, m_intensityByteDepth, m_stateByteDepth;

  // Pointers to the image data
  std::vector<uint16_t> m_distanceMap;
  std::vector<uint16_t> m_intensityMap;
  std::vector<uint16_t> m_stateMap;
};

}
