//
// Copyright note: Redistribution and use in source, with or without modification, are permitted.
// 
// Created: August 2020
// 
// SICK AG, Waldkirch
// email: TechSupport0905@sick.de

#include <cstdio>

#include "VisionaryTMiniData.h"
#include "VisionaryEndian.h"

// TinyXML-2 XML DOM parser
#include "tinyxml2.h"

namespace visionary 
{

const float VisionaryTMiniData::DISTANCE_MAP_UNIT = 0.25f;

VisionaryTMiniData::VisionaryTMiniData() : VisionaryData()
{
}

VisionaryTMiniData::~VisionaryTMiniData()
{
}

bool VisionaryTMiniData::parseXML(const std::string & xmlString, uint32_t changeCounter)
{
  //-----------------------------------------------
  // Check if the segment data changed since last receive
  if (m_changeCounter == changeCounter)
  {
    return true;  //Same XML content as on last received blob
  }
  m_changeCounter = changeCounter;
  m_preCalcCamInfoType = VisionaryData::UNKNOWN;

  //-----------------------------------------------
  // Parse XML string into DOM
  tinyxml2::XMLDocument xmlTree;
  auto tXMLError = xmlTree.Parse(xmlString.c_str());
  if (tXMLError != tinyxml2::XMLError::XML_SUCCESS) {
    std::printf("Reading XML tree in BLOB failed.");
    return false;
  }

  //-----------------------------------------------
  // Check whether datasets entry exists and whether it has a depthmap
  bool oParseOk = true;
  tinyxml2::XMLNode const* ptDataSetsTree = 0;
  {
    ptDataSetsTree = xmlTree.FirstChildElement("SickRecord");

    if (ptDataSetsTree != 0)
    {
      ptDataSetsTree = ptDataSetsTree->FirstChildElement("DataSets");
    }

    if (ptDataSetsTree != 0)
    {
      auto const* ptHasNodeOrZero = ptDataSetsTree->FirstChildElement("DataSetDepthMap");
      m_dataSetsActive.hasDataSetDepthMap = (ptHasNodeOrZero != 0);
    }
    else
    {
      oParseOk = false;
    }
  }

  // DataSetDepthMap specific data 
  if (oParseOk)
  {
    auto const* ptDataStreamTree = ptDataSetsTree->FirstChildElement("DataSetDepthMap");

    if (ptDataStreamTree != 0)
    {
      ptDataStreamTree = ptDataStreamTree->FirstChildElement("FormatDescriptionDepthMap");
    }

    if (ptDataStreamTree != 0)
    {
      ptDataStreamTree = ptDataStreamTree->FirstChildElement("DataStream");
    }
    else
    {
      oParseOk = false;
    }

    // Image width/height
    if (oParseOk and (ptDataStreamTree != 0))
    {
      auto const* ptWidthElem = ptDataStreamTree->FirstChildElement("Width");
      if (ptWidthElem != 0)
      {
        tXMLError = ptWidthElem->QueryIntText(&m_cameraParams.width);
        if (tXMLError != tinyxml2::XMLError::XML_SUCCESS)
        {
          oParseOk = false;
        }
      }
      else
      {
        oParseOk = false;
      }

      auto const* ptHeightElem = ptDataStreamTree->FirstChildElement("Height");
      if (ptHeightElem != 0)
      {
        tXMLError = ptHeightElem->QueryIntText(&m_cameraParams.height);
        if (tXMLError != tinyxml2::XMLError::XML_SUCCESS)
        {
          oParseOk = false;
        }
      }
      else
      {
        oParseOk = false;
      }
    }

    // Camera transformation matrix
    if (m_dataSetsActive.hasDataSetDepthMap and (ptDataStreamTree != 0))
    {
      auto const* ptCameraToWorldTransform = ptDataStreamTree->FirstChildElement("CameraToWorldTransform");
      if (ptCameraToWorldTransform != 0)
      {
        auto const* ptMatrixEntry = ptCameraToWorldTransform->FirstChildElement();
        for (auto idx = 0; idx < 4*4; ++idx)
        {
          tXMLError = ptMatrixEntry->QueryDoubleText(&m_cameraParams.cam2worldMatrix[idx]);
          ptMatrixEntry = ptMatrixEntry->NextSiblingElement();
          if ((tXMLError != tinyxml2::XMLError::XML_SUCCESS) or
              ((idx < 4*4-1) and (ptMatrixEntry == 0)))
          {
            oParseOk = false;
            break;
          }
        }
      }
      else
      {
        oParseOk = false;
      }
    }
    else
    {
      // Identity matrix
      std::fill(m_cameraParams.cam2worldMatrix, 
                m_cameraParams.cam2worldMatrix + 16, 
                0.0);
      m_cameraParams.cam2worldMatrix[ 0] = 1.0;
      m_cameraParams.cam2worldMatrix[ 5] = 1.0;
      m_cameraParams.cam2worldMatrix[10] = 1.0;
      m_cameraParams.cam2worldMatrix[15] = 1.0;
    }

    // Camera intrinsics
    // Lambda: read double-type entry or set to zero
    auto getDoubleEntry = [&](const std::vector<char const*>& path, 
                              double* target) {
      auto const* ptEntry = ptDataStreamTree;
      for (auto* part: path)
      {
        if (ptEntry != 0)
        {
          ptEntry = ptEntry->FirstChildElement(part); 
        }
      }

      if (ptEntry != 0)
      {
        tXMLError = ptEntry->QueryDoubleText(target); 
        if (tXMLError != tinyxml2::XMLError::XML_SUCCESS)
        {
          oParseOk = false;
        }
      }
      else 
      {
        oParseOk = false;
      }
    };
    getDoubleEntry({"CameraMatrix","FX"}, &m_cameraParams.fx);
    getDoubleEntry({"CameraMatrix","FY"}, &m_cameraParams.fy);
    getDoubleEntry({"CameraMatrix","CX"}, &m_cameraParams.cx);
    getDoubleEntry({"CameraMatrix","CY"}, &m_cameraParams.cy);

    getDoubleEntry({"CameraDistortionParams","K1"}, &m_cameraParams.k1);
    getDoubleEntry({"CameraDistortionParams","K2"}, &m_cameraParams.k2);
    getDoubleEntry({"CameraDistortionParams","P1"}, &m_cameraParams.p1);
    getDoubleEntry({"CameraDistortionParams","P2"}, &m_cameraParams.p2);
    getDoubleEntry({"CameraDistortionParams","K3"}, &m_cameraParams.k3);

    getDoubleEntry({"FocalToRayCross"}, &m_cameraParams.f2rc);

    // Read datatypes of image maps
    // Lambda: read text entry or set to empty string
    auto getText = [&](char const* name) {
      if (ptDataStreamTree == 0)
      {
        return "";
      }

      auto const* ptEntry = ptDataStreamTree->FirstChildElement(name);
      if (ptEntry != 0)
      {
        return ptEntry->GetText();
      }
      else
      {
        return "";
      }
    };
    // Lookup byte widths of datatypes
    m_distanceByteDepth  = getItemLength(getText("Distance"));
    m_intensityByteDepth = getItemLength(getText("Intensity"));
    m_stateByteDepth     = getItemLength(getText("Confidence"));

    //const auto distanceDecimalExponent = dataStreamTree.get<int>("Distance.<xmlattr>.decimalexponent", 0);
    // Scaling is fixed to 0.25mm on ToF Mini
    m_scaleZ = DISTANCE_MAP_UNIT;
  }

  return true;
}

bool VisionaryTMiniData::parseBinaryData(std::vector<uint8_t>::iterator itBuf, size_t size)
{
  size_t dataSetslength = 0;

  if (m_dataSetsActive.hasDataSetDepthMap)
  {
    const size_t numPixel = m_cameraParams.width * m_cameraParams.height;
    const size_t numBytesDistance = numPixel * m_distanceByteDepth;
    const size_t numBytesIntensity = numPixel * m_intensityByteDepth;
    const size_t numBytesState = numPixel * m_stateByteDepth;

    //-----------------------------------------------
    // The binary part starts with entries for length, a timestamp
    // and a version identifier
    const uint32_t length = readUnalignLittleEndian<uint32_t>(&*itBuf);
    dataSetslength += length;
    if (dataSetslength > size)
    {
      wprintf(L"Malformed data, length in depth map header does not match package size.");
      return false;
    }
    itBuf += sizeof(uint32_t);

    m_blobTimestamp = readUnalignLittleEndian<uint64_t>(&*itBuf);
    itBuf += sizeof(uint64_t);

    const uint16_t version = readUnalignLittleEndian<uint16_t>(&*itBuf);
    itBuf += sizeof(uint16_t);

    //-----------------------------------------------
    // The content of the Data part inside a data set has changed since the first released version.
    if (version > 1) {
      // more frame information follows in this case: frame number, data quality, device status
      m_frameNum = readUnalignLittleEndian<uint32_t>(&*itBuf);
      itBuf += sizeof(uint32_t);

      //const uint8_t dataQuality = readUnalignLittleEndian<uint8_t>(&*itBuf);
      itBuf += sizeof(uint8_t);

      //const uint8_t deviceStatus = readUnalignLittleEndian<uint8_t>(&*itBuf);
      itBuf += sizeof(uint8_t);
    }
    else {
      ++m_frameNum;
    }

    //-----------------------------------------------
    // Extract the Images depending on the informations extracted from the XML part
    if (numBytesDistance != 0) {
        m_distanceMap.resize(numPixel);
        memcpy(&m_distanceMap[0], &*itBuf, numBytesDistance);
        itBuf += numBytesDistance;
    }
    else {
        m_distanceMap.clear();
    }
    if (numBytesIntensity != 0) {
        m_intensityMap.resize(numPixel);
        memcpy(&m_intensityMap[0], &*itBuf, numBytesIntensity);
        itBuf += numBytesIntensity;
    }
    else {
        m_intensityMap.clear();
    }
    if (numBytesState != 0) {
        m_stateMap.resize(numPixel);
        memcpy(&m_stateMap[0], &*itBuf, numBytesState);
        itBuf += numBytesState;
    }
    else {
        m_stateMap.clear();
    }

    //-----------------------------------------------
    // Data ends with a (unused) 4 Byte CRC field and a copy of the length byte
    //const uint32_t unusedCrc = readUnalignLittleEndian<uint32_t>(&*itBuf);
    itBuf += sizeof(uint32_t);

    const uint32_t lengthCopy = readUnalignLittleEndian<uint32_t>(&*itBuf);
    itBuf += sizeof(uint32_t);

    if (length != lengthCopy)
    {
      wprintf(L"Malformed data, length in header does not match package size.");
      return false;
    }
  }
  else
  {
    m_distanceMap.clear();
    m_intensityMap.clear();
    m_stateMap.clear();
  }

  return true;
}

void VisionaryTMiniData::generatePointCloud(std::vector<PointXYZ> &pointCloud)
{
  return VisionaryData::generatePointCloud(m_distanceMap, VisionaryData::RADIAL, pointCloud);
}

const std::vector<uint16_t>& VisionaryTMiniData::getDistanceMap() const
{
  return m_distanceMap;
}

const std::vector<uint16_t>& VisionaryTMiniData::getIntensityMap() const
{
  return m_intensityMap;
}

const std::vector<uint16_t>& VisionaryTMiniData::getStateMap() const
{
    return m_stateMap;
}

}
