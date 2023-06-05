//
// Copyright note: Redistribution and use in source, with or without modification, are permitted.
// 
// Created: November 2019
// 
// SICK AG, Waldkirch
// email: TechSupport0905@sick.de

#include <cassert>
#include <cstdio>
#include <cmath>

#include "VisionaryTData.h"
#include "VisionaryEndian.h"

// TinyXML-2 XML DOM parser
#include "tinyxml2.h"

namespace visionary 
{

VisionaryTData::VisionaryTData() : VisionaryData()
{
}

VisionaryTData::~VisionaryTData()
{
}

bool VisionaryTData::parseXML(const std::string & xmlString, uint32_t changeCounter)
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

      ptHasNodeOrZero = ptDataSetsTree->FirstChildElement("DataSetPolar2D");
      m_dataSetsActive.hasDataSetPolar2D = (ptHasNodeOrZero != 0);

      ptHasNodeOrZero = ptDataSetsTree->FirstChildElement("DataSetCartesian");
      m_dataSetsActive.hasDataSetCartesian = (ptHasNodeOrZero != 0);
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
    m_distanceByteDepth   = getItemLength(getText("Distance"));
    m_intensityByteDepth  = getItemLength(getText("Intensity"));
    m_confidenceByteDepth = getItemLength(getText("Confidence"));

    auto const* ptProbe = ptDataStreamTree->FirstChildElement("Distance");
    if (ptProbe != 0)
    {
      int i32DistanceDecimalExponent = 0;
      tXMLError = ptProbe->QueryIntAttribute("decimalexponent", 
                                             &i32DistanceDecimalExponent);
      if (tXMLError == tinyxml2::XMLError::XML_SUCCESS)
      {
        m_scaleZ = powf(10.0f, static_cast<float>(i32DistanceDecimalExponent));
      }
    }
  }

  // DataSetPolar2D specific data
  if (oParseOk)
  {
    auto const* ptDataStreamTree = ptDataSetsTree->FirstChildElement("DataSetPolar2D");

    if (ptDataStreamTree != 0)
    {
      ptDataStreamTree = ptDataStreamTree->FirstChildElement("FormatDescriptionDepthMap");
    }

    if (ptDataStreamTree != 0)
    {
      ptDataStreamTree = ptDataStreamTree->FirstChildElement("DataStream");
    }

    if (ptDataStreamTree != 0)
    {
      m_numPolarValues = 0;
      int i32DataLength = 0;
      tXMLError = ptDataStreamTree->QueryIntAttribute("datalength", 
                                                      &i32DataLength);
      if (tXMLError == tinyxml2::XMLError::XML_SUCCESS)
      {
        m_numPolarValues = i32DataLength;
      }
    }
    else
    {
      oParseOk = false;
    }
  }

  // DataSetCartesian specific data
  if (oParseOk and m_dataSetsActive.hasDataSetCartesian)
  {
    auto const* ptDataStreamTree = ptDataSetsTree->FirstChildElement("DataSetCartesian");

    if (ptDataStreamTree != 0)
    {
      ptDataStreamTree = ptDataStreamTree->FirstChildElement("FormatDescriptionDepthMap");
    }

    if (ptDataStreamTree != 0)
    {
      ptDataStreamTree = ptDataStreamTree->FirstChildElement("DataStream");
    }

    if (ptDataStreamTree != 0)
    {
      bool oOk = true;
      auto const* ptChild = ptDataStreamTree->FirstChildElement("Length");
      oOk = (oOk and 
             (ptChild != 0) and 
             (std::strcmp(ptChild->GetText(), "uint32") == 0));

      ptChild = ptDataStreamTree->FirstChildElement("X");
      oOk = (oOk and 
             (ptChild != 0) and 
             (std::strcmp(ptChild->GetText(), "float32") == 0));

      ptChild = ptDataStreamTree->FirstChildElement("Y");
      oOk = (oOk and 
             (ptChild != 0) and 
             (std::strcmp(ptChild->GetText(), "float32") == 0));

      ptChild = ptDataStreamTree->FirstChildElement("Z");
      oOk = (oOk and 
             (ptChild != 0) and 
             (std::strcmp(ptChild->GetText(), "float32") == 0));

      ptChild = ptDataStreamTree->FirstChildElement("Intensity");
      oOk = (oOk and 
             (ptChild != 0) and 
             (std::strcmp(ptChild->GetText(), "float32") == 0));

      if (not oOk)
      {
        std::printf("DataSet Cartesian does not contain the expected format. Won't be used");
        m_dataSetsActive.hasDataSetCartesian = false;
      }
    }

    // To be sure float is 32 bit on this machine, otherwise the parsing of the binary part won't work
    assert(sizeof(float) == 4);
  }

  return true;
}

bool VisionaryTData::parseBinaryData(std::vector<uint8_t>::iterator itBuf, size_t size)
{
  size_t dataSetslength = 0;

  if (m_dataSetsActive.hasDataSetDepthMap)
  {
    const size_t numPixel = m_cameraParams.width * m_cameraParams.height;
    const size_t numBytesDistance = numPixel * m_distanceByteDepth;
    const size_t numBytesIntensity = numPixel * m_intensityByteDepth;
    const size_t numBytesConfidence = numPixel * m_confidenceByteDepth;

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
    m_distanceMap.resize(numPixel);
    memcpy(&m_distanceMap[0], &*itBuf, numBytesDistance);
    itBuf += numBytesDistance;

    m_intensityMap.resize(numPixel);
    memcpy(&m_intensityMap[0], &*itBuf, numBytesIntensity);
    itBuf += numBytesIntensity;

    m_confidenceMap.resize(numPixel);
    memcpy(&m_confidenceMap[0], &*itBuf, numBytesConfidence);
    itBuf += numBytesConfidence;

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
    m_confidenceMap.clear();
  }
  
  if (m_dataSetsActive.hasDataSetPolar2D)
  {
    const uint32_t length = readUnalignLittleEndian<uint32_t>(&*itBuf);
    dataSetslength += length;
    if (dataSetslength > size)
    {
      std::printf("Malformed data, length in polar scan header does not match package size.");
      return false;
    }
    itBuf += sizeof(uint32_t);
    m_blobTimestamp = readUnalignLittleEndian<uint64_t>(&*itBuf);
    itBuf += sizeof(uint64_t);

    //const uint16_t deviceID = readUnalignLittleEndian<uint16_t>(&*itBuf);
    itBuf += sizeof(uint16_t);
    //const uint32_t scanCounter = readUnalignLittleEndian<uint32_t>(&*itBuf);
    itBuf += sizeof(uint32_t);
    //const uint32_t systemCounterScan = readUnalignLittleEndian<uint32_t>(&*itBuf);
    itBuf += sizeof(uint32_t);
    //const float scanFrequency = readUnalignLittleEndian<float>(&*itBuf);
    itBuf += sizeof(float);
    //const float measurementFrequency = readUnalignLittleEndian<float>(&*itBuf);
    itBuf += sizeof(float);

    m_angleFirstScanPoint = readUnalignLittleEndian<float>(&*itBuf);
    itBuf += sizeof(float);
    m_angularResolution = readUnalignLittleEndian<float>(&*itBuf);
    itBuf += sizeof(float);
    //const float polarScale = readUnalignLittleEndian<float>(&*itBuf);
    itBuf += sizeof(float);
    //const float polarOffset = readUnalignLittleEndian<float>(&*itBuf);
    itBuf += sizeof(float);

    m_polarDistanceData.resize(m_numPolarValues);

    memcpy(&m_polarDistanceData[0], &*itBuf, (m_numPolarValues * sizeof(float)));
    itBuf += (m_numPolarValues * sizeof(float));

    //const float rssiAngleFirstScanPoint = readUnalignLittleEndian<float>(&*itBuf);
    itBuf += sizeof(float);
    //const float rssiAngularResolution = readUnalignLittleEndian<float>(&*itBuf);
    itBuf += sizeof(float);
    //const float rssiPolarScale = readUnalignLittleEndian<float>(&*itBuf);
    itBuf += sizeof(float);
    //const float rssiPolarOffset = readUnalignLittleEndian<float>(&*itBuf);
    itBuf += sizeof(float);

    m_polarConfidenceData.resize(m_numPolarValues);
    memcpy(&m_polarConfidenceData[0], &*itBuf, (m_numPolarValues * sizeof(float)));
    itBuf += (m_numPolarValues * sizeof(float));

    //-----------------------------------------------
    // Data ends with a (unused) 4 Byte CRC field and a copy of the length byte
    //const uint32_t unusedCrc = readUnalignLittleEndian<uint32_t>(&*itBuf);
    itBuf += sizeof(uint32_t);

    const uint32_t lengthCopy = readUnalignLittleEndian<uint32_t>(&*itBuf);
    itBuf += sizeof(uint32_t);

    if (length != lengthCopy)
    {
      std::printf("Malformed data, length in header does not match package size.");
      return false;
    }
  }
  else
  {
    m_polarDistanceData.clear();
    m_polarConfidenceData.clear();
  }

  m_numCartesianValues = 0;
  if (m_dataSetsActive.hasDataSetCartesian)
  {
    const uint32_t length = readUnalignLittleEndian<uint32_t>(&*itBuf);
    dataSetslength += length;
    if (dataSetslength > size)
    {
      std::printf("Malformed data, length in cartesian header does not match package size.");
      return false;
    }
    itBuf += sizeof(uint32_t);
    m_blobTimestamp = readUnalignLittleEndian<uint64_t>(&*itBuf);
    itBuf += sizeof(uint64_t);
    //const uint16_t version = readUnalignLittleEndian<uint16_t>(&*itBuf);
    itBuf += sizeof(uint16_t);

    m_numCartesianValues = readUnalignLittleEndian<uint32_t>(&*itBuf);
    itBuf += sizeof(uint32_t);
    
    m_cartesianData.resize(m_numCartesianValues);
    memcpy(&m_cartesianData[0], &*itBuf, (m_numCartesianValues * sizeof(PointXYZC)));
    itBuf += (m_numCartesianValues * sizeof(PointXYZC));

    //-----------------------------------------------
    // Data ends with a (unused) 4 Byte CRC field and a copy of the length byte
    //const uint32_t unusedCrc = readUnalignLittleEndian<uint32_t>(&*itBuf);
    itBuf += sizeof(uint32_t);

    const uint32_t lengthCopy = readUnalignLittleEndian<uint32_t>(&*itBuf);
    itBuf += sizeof(uint32_t);

    if (length != lengthCopy)
    {
      std::printf("Malformed data, length in header does not match package size.");
      return false;
    }
  }
  else
  {
    m_cartesianData.clear();
  }

  return true;
}

void VisionaryTData::generatePointCloud(std::vector<PointXYZ> &pointCloud)
{
  return VisionaryData::generatePointCloud(m_distanceMap, VisionaryData::RADIAL, pointCloud);
}

const std::vector<uint16_t>& VisionaryTData::getDistanceMap() const
{
  return m_distanceMap;
}

const std::vector<uint16_t>& VisionaryTData::getIntensityMap() const
{
  return m_intensityMap;
}

const std::vector<uint16_t>& VisionaryTData::getConfidenceMap() const
{
  return m_confidenceMap;
}

uint8_t VisionaryTData::getPolarSize() const
{
  return m_numPolarValues;
}

float VisionaryTData::getPolarStartAngle() const
{
    return m_angleFirstScanPoint;
}

float VisionaryTData::getPolarAngularResolution() const
{
    return m_angularResolution;
}

const std::vector<float>& VisionaryTData::getPolarDistanceData() const
{
  return m_polarDistanceData;
}

const std::vector<float>& VisionaryTData::getPolarConfidenceData() const
{
  return m_polarConfidenceData;
}

uint32_t VisionaryTData::getCartesianSize() const
{
  return m_numCartesianValues;
}

const std::vector<PointXYZC>& VisionaryTData::getCartesianData()const
{
  return m_cartesianData;
}

}
