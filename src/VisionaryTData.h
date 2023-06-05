//
// Copyright note: Redistribution and use in source, with or without modification, are permitted.
// 
// Created: August 2017
// 
// SICK AG, Waldkirch
// email: TechSupport0905@sick.de

#pragma once

#include <string>
#include <vector>

#include "VisionaryData.h"

namespace visionary 
{

typedef struct {
  float angleFirstScanPoint;
  float angularResolution;
  float polarScale;
  float polarOffset;
} PolarParameters;

class VisionaryTData : public VisionaryData
{
public:
  VisionaryTData();
  ~VisionaryTData();

  //-----------------------------------------------
  // Getter Functions
  const std::vector<uint16_t>& getDistanceMap() const;
  const std::vector<uint16_t>& getIntensityMap() const;
  const std::vector<uint16_t>& getConfidenceMap() const;
  // Returns Number of points get by the polar reduction.
  // 0 when no data is available.
  uint8_t getPolarSize() const;
  float getPolarStartAngle() const;
  float getPolarAngularResolution() const;
  const std::vector<float>& getPolarDistanceData() const;
  const std::vector<float>& getPolarConfidenceData() const;
  // Returns Number of points get by the cartesian reduction.
  // 0 when no data is available.
  uint32_t getCartesianSize() const;
  const std::vector<PointXYZC>& getCartesianData() const;
 
  // Calculate and return the Point Cloud in the camera perspective. Units are in meters.
  void generatePointCloud(std::vector<PointXYZ> &pointCloud);

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
  int m_distanceByteDepth, m_intensityByteDepth, m_confidenceByteDepth;

  // Angle information of polar scan
  float m_angleFirstScanPoint;
  float m_angularResolution;

  // Number of values for polar data reduction
  uint_fast8_t m_numPolarValues;
  // Number of values for cartesian data reduction
  uint_fast32_t m_numCartesianValues;

  // Pointers to the image data
  std::vector<uint16_t> m_distanceMap;
  std::vector<uint16_t> m_intensityMap;
  std::vector<uint16_t> m_confidenceMap;
  std::vector<float> m_polarDistanceData;
  std::vector<float> m_polarConfidenceData;
  std::vector<PointXYZC> m_cartesianData;
};

}
