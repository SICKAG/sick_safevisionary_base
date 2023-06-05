//
// Copyright note: Redistribution and use in source, with or without modification, are permitted.
// 
// Created: November 2019
// 
// SICK AG, Waldkirch
// email: TechSupport0905@sick.de

#include <cstdio>
#include <cmath>

#include "VisionarySData.h"
#include "VisionaryEndian.h"

// TinyXML-2 XML DOM parser
#include "tinyxml2.h"

namespace visionary 
{

VisionarySData::VisionarySData() : VisionaryData()
{
}

VisionarySData::~VisionarySData()
{
}

bool VisionarySData::parseXML(const std::string & xmlString, uint32_t changeCounter)
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

    if (ptDataSetsTree == 0)
    {
      oParseOk = false;
    }
  }

  // Identity matrix
  std::fill(m_cameraParams.cam2worldMatrix, 
            m_cameraParams.cam2worldMatrix + 16, 
            0.0);
  m_cameraParams.cam2worldMatrix[ 0] = 1.0;
  m_cameraParams.cam2worldMatrix[ 5] = 1.0;
  m_cameraParams.cam2worldMatrix[10] = 1.0;
  m_cameraParams.cam2worldMatrix[15] = 1.0;

  //-----------------------------------------------
  // DataSetStereo specific data 
  tinyxml2::XMLElement const* ptDataStreamTree = 0;
  if (oParseOk)
  {
    ptDataStreamTree = ptDataSetsTree->FirstChildElement("DataSetStereo");

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
  }

  if (oParseOk)
  {
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
    m_zByteDepth          = getItemLength(getText("Z"));
    m_rgbaByteDepth       = getItemLength(getText("Intensity"));
    m_confidenceByteDepth = getItemLength(getText("Confidence"));

    auto const* ptProbe = ptDataStreamTree->FirstChildElement("Z");
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

  if (not oParseOk)
  {
    m_cameraParams.width  = 0;
    m_cameraParams.height = 0;

    m_cameraParams.fx = 0.0;
    m_cameraParams.fy = 0.0;
    m_cameraParams.cx = 0.0;
    m_cameraParams.cy = 0.0;

    m_cameraParams.k1 = 0.0;
    m_cameraParams.k2 = 0.0;
    m_cameraParams.p1 = 0.0;
    m_cameraParams.p2 = 0.0;
    m_cameraParams.k3 = 0.0;

    m_cameraParams.f2rc = 0.0;

    m_zByteDepth          = getItemLength("");
    m_rgbaByteDepth       = getItemLength("");
    m_confidenceByteDepth = getItemLength("");

    m_scaleZ = 1.0;

    // Identity matrix
    std::fill(m_cameraParams.cam2worldMatrix, 
              m_cameraParams.cam2worldMatrix + 16, 
              0.0);
    m_cameraParams.cam2worldMatrix[ 0] = 1.0;
    m_cameraParams.cam2worldMatrix[ 5] = 1.0;
    m_cameraParams.cam2worldMatrix[10] = 1.0;
    m_cameraParams.cam2worldMatrix[15] = 1.0;
  }

  return true;
}

bool VisionarySData::parseBinaryData(std::vector<uint8_t>::iterator itBuf, size_t size)
{
  const size_t numPixel = m_cameraParams.width * m_cameraParams.height;
  const size_t numBytesZ = numPixel * m_zByteDepth;
  const size_t numBytesRGBA = numPixel * m_rgbaByteDepth;
  const size_t numBytesConfidence = numPixel * m_confidenceByteDepth;


  //-----------------------------------------------
  // The binary part starts with entries for length, a timestamp
  // and a version identifier
    const uint32_t length = readUnalignLittleEndian<uint32_t>(&*itBuf);
    if (length > size)
    {
      std::printf("Malformed data, length in depth map header does not match package size.");
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
    m_zMap.resize(numPixel);
    memcpy(&m_zMap[0], &*itBuf, numBytesZ);
    itBuf += numBytesZ;

    m_rgbaMap.resize(numPixel);
    memcpy(&m_rgbaMap[0], &*itBuf, numBytesRGBA);
    itBuf += numBytesRGBA;

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
      std::printf("Malformed data, length in header does not match package size.");
      return false;
    }

  return true;
}

void VisionarySData::generatePointCloud(std::vector<PointXYZ> &pointCloud)
{
  return VisionaryData::generatePointCloud(m_zMap, VisionaryData::PLANAR, pointCloud);
}

const std::vector<uint16_t>& VisionarySData::getZMap() const
{
  return m_zMap;
}

const std::vector<uint32_t>& VisionarySData::getRGBAMap() const
{
  return m_rgbaMap;
}

const std::vector<uint16_t>& VisionarySData::getConfidenceMap() const
{
  return m_confidenceMap;
}

}
