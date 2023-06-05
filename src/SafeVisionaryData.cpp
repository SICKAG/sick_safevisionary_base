//
// Copyright note: Redistribution and use in source, with or without modification, are permitted.
//
// Created: December 2021
//
// SICK AG, Waldkirch
// email: TechSupport0905@sick.de

#include <cstdio>

#include "sick_safevisionary_base/CRC.h"
#include "sick_safevisionary_base/SafeVisionaryData.h"
#include "sick_safevisionary_base/VisionaryEndian.h"
#include <iostream>
// TinyXML-2 XML DOM parser
#include "sick_safevisionary_base/tinyxml2.h"

namespace visionary
{

constexpr float SafeVisionaryData::DISTANCE_MAP_UNIT = 0.25f;

namespace
{
/// Version number of the Blob segment DepthMap
constexpr std::uint16_t VERSION_SEGMENT_DEPTHMAP = 0x0002u;

/// Version number of the Blob segment ROI
constexpr std::uint16_t VERSION_SEGMENT_DEVICESTATUS = 0x0001u;

/// Version number of the Blob segment ROI
constexpr std::uint16_t VERSION_SEGMENT_ROI = 0x0001u;

/// Version number of the Blob segment Local I/Os
constexpr std::uint16_t VERSION_SEGMENT_LOCALIOS = 0x0001u;

/// Version number of the Blob segment Field Information
constexpr std::uint16_t VERSION_SEGMENT_FIELDINFORMATION = 0x0001u;

/// Version number of the Blob segment Logic Signal
constexpr std::uint16_t VERSION_SEGMENT_LOGICSIGNALS = 0x0001u;

/// Version number of the Blob segment IMU
constexpr std::uint16_t VERSION_SEGMENT_IMU = 0x0001u;

/// Flag whether the unfiltered or filtered distance map is received */
constexpr std::uint16_t DISTANCE_MAP_FILTERED_FLAG = 1u << 0;

/** Flag whether the intrusion data is valid in the pixel state map */
constexpr std::uint16_t INTRUDED_PIXEL_STATE_VALID_FLAG = 1u << 1;

/** Flag whether data stream is throttled or not  */
constexpr std::uint16_t DATA_STREAM_THROTTLED_FLAG = 1u << 2;

} // namespace
SafeVisionaryData::SafeVisionaryData()
  : VisionaryData()
  , m_distanceByteDepth(0u)
  , m_intensityByteDepth(0u)
  , m_stateByteDepth(0u)
  , m_deviceStatus(DEVICE_STATUS::DEVICE_STATUS_INVALID)
  , m_flags(0u)
  , m_lastDataHandlerError(DataHandlerError::OK)
{
}

SafeVisionaryData::~SafeVisionaryData()
{
}

bool SafeVisionaryData::parseXML(const std::string& xmlString, uint32_t changeCounter)
{
  //-----------------------------------------------
  // Check if the segment data changed since last receive
  if (m_changeCounter == changeCounter)
  {
    return true; // Same XML content as on last received blob
  }

  m_preCalcCamInfoType = VisionaryData::UNKNOWN;

  //-----------------------------------------------
  // Parse XML string into DOM
  tinyxml2::XMLDocument xmlTree;
  auto                  tXMLError = xmlTree.Parse(xmlString.c_str());
  if (tXMLError != tinyxml2::XMLError::XML_SUCCESS)
  {
    std::printf("Reading XML tree in BLOB failed.");
    m_lastDataHandlerError = DataHandlerError::PARSE_XML_ERROR;
    return false;
  }

  //-----------------------------------------------
  // Check whether datasets entry exists and whether it has a depthmap
  bool                     oParseOk       = true;
  tinyxml2::XMLNode const* ptDataSetsTree = 0;
  {
    ptDataSetsTree = xmlTree.FirstChildElement("SickRecord");

    if (ptDataSetsTree != 0)
    {
      ptDataSetsTree = ptDataSetsTree->FirstChildElement("DataSets");
    }

    if (ptDataSetsTree != 0)
    {
      auto const* ptHasNodeOrZero         = ptDataSetsTree->FirstChildElement("DataSetDepthMap");
      m_dataSetsActive.hasDataSetDepthMap = (ptHasNodeOrZero != 0);

      ptHasNodeOrZero                         = ptDataSetsTree->FirstChildElement("DataSetDeviceStatus");
      m_dataSetsActive.hasDataSetDeviceStatus = (ptHasNodeOrZero != 0);

      ptHasNodeOrZero                = ptDataSetsTree->FirstChildElement("DataSetROI");
      m_dataSetsActive.hasDataSetROI = (ptHasNodeOrZero != 0);

      ptHasNodeOrZero                     = ptDataSetsTree->FirstChildElement("DataSetLocalIOs");
      m_dataSetsActive.hasDataSetLocalIOs = (ptHasNodeOrZero != 0);

      ptHasNodeOrZero                      = ptDataSetsTree->FirstChildElement("DataSetFieldInformation");
      m_dataSetsActive.hasDataSetFieldInfo = (ptHasNodeOrZero != 0);

      ptHasNodeOrZero                         = ptDataSetsTree->FirstChildElement("DataSetLogicalSignals");
      m_dataSetsActive.hasDataSetLogicSignals = (ptHasNodeOrZero != 0);

      ptHasNodeOrZero                = ptDataSetsTree->FirstChildElement("DataSetIMU");
      m_dataSetsActive.hasDataSetIMU = (ptHasNodeOrZero != 0);
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
        for (auto idx = 0; idx < 4 * 4; ++idx)
        {
          tXMLError     = ptMatrixEntry->QueryDoubleText(&m_cameraParams.cam2worldMatrix[idx]);
          ptMatrixEntry = ptMatrixEntry->NextSiblingElement();
          if ((tXMLError != tinyxml2::XMLError::XML_SUCCESS) or ((idx < 4 * 4 - 1) and (ptMatrixEntry == 0)))
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
      std::fill(m_cameraParams.cam2worldMatrix, m_cameraParams.cam2worldMatrix + 16, 0.0);
      m_cameraParams.cam2worldMatrix[0]  = 1.0;
      m_cameraParams.cam2worldMatrix[5]  = 1.0;
      m_cameraParams.cam2worldMatrix[10] = 1.0;
      m_cameraParams.cam2worldMatrix[15] = 1.0;
    }

    // Camera intrinsics
    // Lambda: read double-type entry or set to zero
    auto getDoubleEntry = [&](const std::vector<char const*>& path, double* target) {
      auto const* ptEntry = ptDataStreamTree;
      for (auto* part : path)
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
    getDoubleEntry({"CameraMatrix", "FX"}, &m_cameraParams.fx);
    getDoubleEntry({"CameraMatrix", "FY"}, &m_cameraParams.fy);
    getDoubleEntry({"CameraMatrix", "CX"}, &m_cameraParams.cx);
    getDoubleEntry({"CameraMatrix", "CY"}, &m_cameraParams.cy);

    getDoubleEntry({"CameraDistortionParams", "K1"}, &m_cameraParams.k1);
    getDoubleEntry({"CameraDistortionParams", "K2"}, &m_cameraParams.k2);
    getDoubleEntry({"CameraDistortionParams", "P1"}, &m_cameraParams.p1);
    getDoubleEntry({"CameraDistortionParams", "P2"}, &m_cameraParams.p2);
    getDoubleEntry({"CameraDistortionParams", "K3"}, &m_cameraParams.k3);

    // Set f2rc to 0 as the f2rc value is already included in the cameraToWorldMatrix
    // NOTE: this does NOT hold for the yellow device; SafeVisionary has to use that entry
    getDoubleEntry({"FocalToRayCross"}, &m_cameraParams.f2rc);
    // m_cameraParams.f2rc = 0u;

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

    // const auto distanceDecimalExponent = dataStreamTree.get<int>("Distance.<xmlattr>.decimalexponent", 0);
    // Scaling is fixed to 0.25mm
    m_scaleZ = DISTANCE_MAP_UNIT;
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

    m_distanceByteDepth  = getItemLength("");
    m_intensityByteDepth = getItemLength("");
    m_stateByteDepth     = getItemLength("");

    // Identity matrix
    std::fill(m_cameraParams.cam2worldMatrix, m_cameraParams.cam2worldMatrix + 16, 0.0);
    m_cameraParams.cam2worldMatrix[0]  = 1.0;
    m_cameraParams.cam2worldMatrix[5]  = 1.0;
    m_cameraParams.cam2worldMatrix[10] = 1.0;
    m_cameraParams.cam2worldMatrix[15] = 1.0;
  }

  m_changeCounter = changeCounter;

  return true;
}

bool SafeVisionaryData::parseBinaryData(std::vector<uint8_t>::iterator itBuf, size_t size)
{
  const size_t numPixel          = m_cameraParams.width * m_cameraParams.height;
  const size_t numBytesDistance  = numPixel * m_distanceByteDepth;
  const size_t numBytesIntensity = numPixel * m_intensityByteDepth;
  const size_t numBytesState     = numPixel * m_stateByteDepth;

  // get length of data segment DepthMap
  const uint32_t length = readUnalignLittleEndian<uint32_t>(&*itBuf);
  itBuf += sizeof(uint32_t);

  //-----------------------------------------------
  // Data ends with a CRC32 field and a copy of the length byte
  const uint32_t dataSize        = length - 8u;
  const uint32_t crc32           = readUnalignLittleEndian<uint32_t>(&*(itBuf + dataSize));
  const uint32_t crc32Calculated = ~CRC_calcCrc32Block(&*itBuf, dataSize, CRC_DEFAULT_INIT_VALUE32);

  if (crc32 != crc32Calculated)
  {
    std::printf("Malformed data, CRC32 checksum in data segment depth map does not match.\n");
    m_lastDataHandlerError = DataHandlerError::INVALID_CRC_SEGMENT_DEPTHMAP;
    return false;
  }

  // length does not include the second length field, so add 4 bytes
  if ((length + sizeof(uint32_t)) != size)
  {
    std::printf("Malformed data, length in data segment depth map header does not match package size.\n");
    m_lastDataHandlerError = DataHandlerError::INVALID_LENGTH_SEGMENT_DEPTHMAP;
    return false;
  }

  // get second length field
  const uint32_t lengthCopy = readUnalignLittleEndian<uint32_t>(&*(itBuf + dataSize + 4));

  // check whether length matches
  if (length != lengthCopy)
  {
    std::printf("Malformed data, length of data segment depth map header does not match data segment size.\n");
    m_lastDataHandlerError = DataHandlerError::INVALID_LENGTH_SEGMENT_DEPTHMAP;
    return false;
  }

  m_blobTimestamp                      = readUnalignLittleEndian<uint64_t>(&*itBuf);
  m_segmentTimestamp[DEPTHMAP_SEGMENT] = m_blobTimestamp;
  itBuf += sizeof(uint64_t);

  const uint16_t version = readUnalignLittleEndian<uint16_t>(&*itBuf);
  itBuf += sizeof(uint16_t);

  if (version != VERSION_SEGMENT_DEPTHMAP)
  {
    std::printf("Unsupported version of data segment Depthmap\n");
    m_lastDataHandlerError = DataHandlerError::INVALID_VERSION_SEGMENT_DEPTHMAP;
    return false;
  }

  // more frame information follows in this case: frame number, device status and flags
  m_frameNum = readUnalignLittleEndian<uint32_t>(&*itBuf);
  itBuf += sizeof(uint32_t);

  m_deviceStatus = static_cast<DEVICE_STATUS>(readUnalignLittleEndian<uint8_t>(&*itBuf));
  itBuf += sizeof(uint8_t);

  m_flags = readUnalignLittleEndian<uint16_t>(&*itBuf);
  itBuf += sizeof(uint16_t);

  //-----------------------------------------------
  // Extract the Images depending on the informations extracted from the XML part
  if (numBytesDistance != 0)
  {
    m_distanceMap.resize(numPixel);
    memcpy(&m_distanceMap[0], &*itBuf, numBytesDistance);
    itBuf += numBytesDistance;
  }
  else
  {
    m_distanceMap.clear();
  }
  if (numBytesIntensity != 0)
  {
    m_intensityMap.resize(numPixel);
    memcpy(&m_intensityMap[0], &*itBuf, numBytesIntensity);
    itBuf += numBytesIntensity;
  }
  else
  {
    m_intensityMap.clear();
  }
  if (numBytesState != 0)
  {
    m_stateMap.resize(numPixel);
    memcpy(&m_stateMap[0], &*itBuf, numBytesState);
    itBuf += numBytesState;
  }
  else
  {
    m_stateMap.clear();
  }

  return true;
}

bool SafeVisionaryData::parseRoiData(std::vector<uint8_t>::iterator itBuf, size_t size)
{
  // get length of data segment ROI
  const uint32_t length = readUnalignLittleEndian<uint32_t>(&*itBuf);
  itBuf += sizeof(uint32_t);

  //-----------------------------------------------
  // Data ends with a CRC32 field and a copy of the length byte
  const uint32_t dataSize        = length - 8u;
  const uint32_t crc32           = readUnalignLittleEndian<uint32_t>(&*(itBuf + dataSize));
  const uint32_t crc32Calculated = ~CRC_calcCrc32Block(&*itBuf, dataSize, CRC_DEFAULT_INIT_VALUE32);

  if (crc32 != crc32Calculated)
  {
    std::printf("Malformed data, CRC32 checksum of data segment ROI does not match.\n");
    m_lastDataHandlerError = DataHandlerError::INVALID_CRC_SEGMENT_ROI;
    return false;
  }

  // length does not include the second length field, so add 4 bytes
  if ((length + sizeof(uint32_t)) != size)
  {
    std::printf("Malformed data, length of data segment ROI does not match package size.\n");
    m_lastDataHandlerError = DataHandlerError::INVALID_LENGTH_SEGMENT_ROI;
    return false;
  }

  // get second length field
  const uint32_t lengthCopy = readUnalignLittleEndian<uint32_t>(&*(itBuf + dataSize + 4));

  // check whether length matches
  if (length != lengthCopy)
  {
    std::printf("Malformed data, length does not match ROI data segment size.\n");
    m_lastDataHandlerError = DataHandlerError::INVALID_LENGTH_SEGMENT_ROI;
    return false;
  }

  m_blobTimestamp                 = readUnalignLittleEndian<uint64_t>(&*itBuf);
  m_segmentTimestamp[ROI_SEGMENT] = m_blobTimestamp;
  itBuf += sizeof(uint64_t);

  const uint16_t version = readUnalignLittleEndian<uint16_t>(&*itBuf);
  itBuf += sizeof(uint16_t);

  if (version != VERSION_SEGMENT_ROI)
  {
    std::printf("Unsupported version of data segment ROI\n");
    m_lastDataHandlerError = DataHandlerError::INVALID_VERSION_SEGMENT_ROI;
    return false;
  }

  // copy ROI data
  memcpy(&m_roiData, &*itBuf, sizeof(m_roiData));

  return true;
}

bool SafeVisionaryData::parseDeviceStatusData(std::vector<uint8_t>::iterator itBuf, size_t size)
{
  // get length of data segment Device Status
  const uint32_t length = readUnalignLittleEndian<uint32_t>(&*itBuf);
  itBuf += sizeof(uint32_t);

  //-----------------------------------------------
  // Data ends with a CRC32 field and a copy of the length byte
  const uint32_t dataSize        = length - 8u;
  const uint32_t crc32           = readUnalignLittleEndian<uint32_t>(&*(itBuf + dataSize));
  const uint32_t crc32Calculated = ~CRC_calcCrc32Block(&*itBuf, dataSize, CRC_DEFAULT_INIT_VALUE32);

  if (crc32 != crc32Calculated)
  {
    std::printf("Malformed data, CRC32 checksum of data segment Device Status does not match.\n");
    m_lastDataHandlerError = DataHandlerError::INVALID_CRC_SEGMENT_DEVICESTATUS;
    return false;
  }

  // length does not include the second length field, so add 4 bytes
  if ((length + sizeof(uint32_t)) != size)
  {
    std::printf("Malformed data, length of Device Status header does not match package size.\n");
    m_lastDataHandlerError = DataHandlerError::INVALID_LENGTH_SEGMENT_DEVICESTATUS;
    return false;
  }

  // get second length field
  const uint32_t lengthCopy = readUnalignLittleEndian<uint32_t>(&*(itBuf + dataSize + 4));

  // check whether length matches
  if (length != lengthCopy)
  {
    std::printf("Malformed data, length does not match Device Status data segment size.\n");
    m_lastDataHandlerError = DataHandlerError::INVALID_LENGTH_SEGMENT_DEVICESTATUS;
    return false;
  }

  m_blobTimestamp                          = readUnalignLittleEndian<uint64_t>(&*itBuf);
  m_segmentTimestamp[DEVICESTATUS_SEGMENT] = m_blobTimestamp;
  itBuf += sizeof(uint64_t);

  const uint16_t version = readUnalignLittleEndian<uint16_t>(&*itBuf);
  itBuf += sizeof(uint16_t);

  if (version != VERSION_SEGMENT_DEVICESTATUS)
  {
    std::printf("Unsupported version of data segment Device Status\n");
    m_lastDataHandlerError = DataHandlerError::INVALID_VERSION_SEGMENT_DEVICESTATUS;
    return false;
  }

  // copy device status data
  memcpy(&m_deviceStatusData, &*itBuf, sizeof(m_deviceStatusData));

  return true;
}

bool SafeVisionaryData::parseLocalIOsData(std::vector<uint8_t>::iterator itBuf, size_t size)
{
  // get length of data segment Local I/Os
  const uint32_t length = readUnalignLittleEndian<uint32_t>(&*itBuf);
  itBuf += sizeof(uint32_t);

  //-----------------------------------------------
  // Data ends with a CRC32 field and a copy of the length byte
  const uint32_t dataSize        = length - 8u;
  const uint32_t crc32           = readUnalignLittleEndian<uint32_t>(&*(itBuf + dataSize));
  const uint32_t crc32Calculated = ~CRC_calcCrc32Block(&*itBuf, dataSize, CRC_DEFAULT_INIT_VALUE32);

  if (crc32 != crc32Calculated)
  {
    std::printf("Malformed data, CRC32 checksum of data segment Device Status does not match.\n");
    m_lastDataHandlerError = DataHandlerError::INVALID_CRC_SEGMENT_LOCALIOS;
    return false;
  }

  // length does not include the second length field, so add 4 bytes
  if ((length + sizeof(uint32_t)) != size)
  {
    std::printf("Malformed data, length of Device Status header does not match package size.\n");
    m_lastDataHandlerError = DataHandlerError::INVALID_LENGTH_SEGMENT_LOCALIOS;
    return false;
  }

  // get second length field
  const uint32_t lengthCopy = readUnalignLittleEndian<uint32_t>(&*(itBuf + dataSize + 4));

  // check whether length matches
  if (length != lengthCopy)
  {
    std::printf("Malformed data, length does not match Local I/Os data segment size.\n");
    m_lastDataHandlerError = DataHandlerError::INVALID_LENGTH_SEGMENT_LOCALIOS;
    return false;
  }

  m_blobTimestamp                      = readUnalignLittleEndian<uint64_t>(&*itBuf);
  m_segmentTimestamp[LOCALIOS_SEGMENT] = m_blobTimestamp;
  itBuf += sizeof(uint64_t);

  const uint16_t version = readUnalignLittleEndian<uint16_t>(&*itBuf);
  itBuf += sizeof(uint16_t);

  if (version != VERSION_SEGMENT_LOCALIOS)
  {
    std::printf("Unsupported version of data segment Local IO\n");
    m_lastDataHandlerError = DataHandlerError::INVALID_VERSION_SEGMENT_LOCALIOS;
    return false;
  }

  // copy Local I/Os data
  memcpy(&m_localIOsData, &*itBuf, sizeof(m_localIOsData));
  return true;
}
//
bool SafeVisionaryData::parseFieldInformationData(std::vector<uint8_t>::iterator itBuf, size_t size)
{
  // get length of data segment FieldInformation
  const uint32_t length = readUnalignLittleEndian<uint32_t>(&*itBuf);
  itBuf += sizeof(uint32_t);

  //-----------------------------------------------
  // Data ends with a CRC32 field and a copy of the length byte
  const uint32_t dataSize        = length - 8u;
  const uint32_t crc32           = readUnalignLittleEndian<uint32_t>(&*(itBuf + dataSize));
  const uint32_t crc32Calculated = ~CRC_calcCrc32Block(&*itBuf, dataSize, CRC_DEFAULT_INIT_VALUE32);

  if (crc32 != crc32Calculated)
  {
    std::printf("Malformed data, CRC32 checksum of data segment Field Information does not match.\n");
    m_lastDataHandlerError = DataHandlerError::INVALID_CRC_SEGMENT_FIELDINFORMATION;
    return false;
  }

  // length does not include the second length field, so add 4 bytes
  if ((length + sizeof(uint32_t)) != size)
  {
    std::printf("Malformed data, length of data segment Field Information does not match package size.\n");
    m_lastDataHandlerError = DataHandlerError::INVALID_LENGTH_SEGMENT_FIELDINFORMATION;
    return false;
  }

  // get second length field
  const uint32_t lengthCopy = readUnalignLittleEndian<uint32_t>(&*(itBuf + dataSize + 4));

  // check whether length matches
  if (length != lengthCopy)
  {
    std::printf("Malformed data, length does not match Field Information data segment size.\n");
    m_lastDataHandlerError = DataHandlerError::INVALID_LENGTH_SEGMENT_FIELDINFORMATION;
    return false;
  }

  m_blobTimestamp                              = readUnalignLittleEndian<uint64_t>(&*itBuf);
  m_segmentTimestamp[FIELDINFORMATION_SEGMENT] = m_blobTimestamp;

  itBuf += sizeof(uint64_t);

  const uint16_t version = readUnalignLittleEndian<uint16_t>(&*itBuf);
  itBuf += sizeof(uint16_t);

  if (version != VERSION_SEGMENT_FIELDINFORMATION)
  {
    std::printf("Unsupported version of data segment Field Information\n");
    m_lastDataHandlerError = DataHandlerError::INVALID_VERSION_SEGMENT_FIELDINFORMATION;
    return false;
  }

  // copy Field Information data
  memcpy(&m_fieldInformationData, &*itBuf, sizeof(m_fieldInformationData));

  return true;
}

bool SafeVisionaryData::parseLogicSignalsData(std::vector<uint8_t>::iterator itBuf, size_t size)
{
  // get length of data segment Logic Signals
  const uint32_t length = readUnalignLittleEndian<uint32_t>(&*itBuf);

  itBuf += sizeof(uint32_t);

  //-----------------------------------------------
  // Data ends with a CRC32 field and a copy of the length byte
  const uint32_t dataSize        = length - 8u;
  const uint32_t crc32           = readUnalignLittleEndian<uint32_t>(&*(itBuf + dataSize));
  const uint32_t crc32Calculated = ~CRC_calcCrc32Block(&*itBuf, dataSize, CRC_DEFAULT_INIT_VALUE32);

  if (crc32 != crc32Calculated)
  {
    std::printf("Malformed data, CRC32 checksum of data segment Logic Signals does not match.\n");
    m_lastDataHandlerError = DataHandlerError::INVALID_CRC_SEGMENT_LOGICSIGNALS;
    return false;
  }

  // length does not include the second length field, so add 4 bytes
  if ((length + sizeof(uint32_t)) != size)
  {
    std::printf("Malformed data, length of data segment Logic Signals does not match package size.\n");
    m_lastDataHandlerError = DataHandlerError::INVALID_LENGTH_SEGMENT_LOGICSIGNALS;
    return false;
  }

  // get second length field
  const uint32_t lengthCopy = readUnalignLittleEndian<uint32_t>(&*(itBuf + dataSize + 4));

  // check whether length matches
  if (length != lengthCopy)
  {
    std::printf("Malformed data, length does not match Logic Signals  data segment size.\n");
    m_lastDataHandlerError = DataHandlerError::INVALID_LENGTH_SEGMENT_LOGICSIGNALS;
    return false;
  }

  m_blobTimestamp                          = readUnalignLittleEndian<uint64_t>(&*itBuf);
  m_segmentTimestamp[LOGICSIGNALS_SEGMENT] = m_blobTimestamp;
  itBuf += sizeof(uint64_t);

  const uint16_t version = readUnalignLittleEndian<uint16_t>(&*itBuf);
  itBuf += sizeof(uint16_t);

  if (version != VERSION_SEGMENT_LOGICSIGNALS)
  {
    std::printf("Unsupported version of data segment Logic Signals \n");
    m_lastDataHandlerError = DataHandlerError::INVALID_VERSION_SEGMENT_LOGICSIGNALS;
    return false;
  }

  // copy Logic Signal data
  memcpy(&m_logicSignalsData, &*itBuf, sizeof(m_logicSignalsData));

  return true;
}

bool SafeVisionaryData::parseIMUData(std::vector<uint8_t>::iterator itBuf, size_t size)
{
  // get length of data segment IMU
  const uint32_t length = readUnalignLittleEndian<uint32_t>(&*itBuf);

  itBuf += sizeof(uint32_t);

  //-----------------------------------------------
  // Data ends with a CRC32 field and a copy of the length byte
  const uint32_t dataSize        = length - 8u;
  const uint32_t crc32           = readUnalignLittleEndian<uint32_t>(&*(itBuf + dataSize));
  const uint32_t crc32Calculated = ~CRC_calcCrc32Block(&*itBuf, dataSize, CRC_DEFAULT_INIT_VALUE32);

  if (crc32 != crc32Calculated)
  {
    std::printf("Malformed data, CRC32 checksum of data segment IMU does not match.\n");
    m_lastDataHandlerError = DataHandlerError::INVALID_CRC_SEGMENT_IMU;
    return false;
  }

  // length does not include the second length field, so add 4 bytes
  if ((length + sizeof(uint32_t)) != size)
  {
    std::printf("Malformed data, length of data segment IMU does not match package size.\n");
    m_lastDataHandlerError = DataHandlerError::INVALID_LENGTH_SEGMENT_IMU;
    return false;
  }

  // get second length field
  const uint32_t lengthCopy = readUnalignLittleEndian<uint32_t>(&*(itBuf + dataSize + 4));

  // check whether length matches
  if (length != lengthCopy)
  {
    std::printf("Malformed data, length does not match IMU data segment size.\n");
    m_lastDataHandlerError = DataHandlerError::INVALID_LENGTH_SEGMENT_IMU;
    return false;
  }

  m_blobTimestamp = readUnalignLittleEndian<uint64_t>(&*itBuf);
  m_segmentTimestamp[IMU_SEGMENT] = m_blobTimestamp;
  itBuf += sizeof(uint64_t);

  const uint16_t version = readUnalignLittleEndian<uint16_t>(&*itBuf);
  itBuf += sizeof(uint16_t);

  if (version != VERSION_SEGMENT_IMU)
  {
    std::printf("Unsupported version of data segment IMU \n");
    m_lastDataHandlerError = DataHandlerError::INVALID_VERSION_SEGMENT_IMU;
    return false;
  }

  // copy IMU data
  memcpy(&m_IMUData, &*itBuf, sizeof(m_IMUData));

  return true;
}

//
void SafeVisionaryData::generatePointCloud(std::vector<PointXYZ>& pointCloud)
{
  return VisionaryData::generatePointCloud(m_distanceMap, VisionaryData::RADIAL, pointCloud);
}

const std::vector<uint16_t>& SafeVisionaryData::getDistanceMap() const
{
  return m_distanceMap;
}

const std::vector<uint16_t>& SafeVisionaryData::getIntensityMap() const
{
  return m_intensityMap;
}

const std::vector<uint8_t>& SafeVisionaryData::getStateMap() const
{
  return m_stateMap;
}

DEVICE_STATUS SafeVisionaryData::getDeviceStatus() const
{
  return m_deviceStatus;
}

bool SafeVisionaryData::isDistanceMapFiltered() const
{
  return (m_flags & DISTANCE_MAP_FILTERED_FLAG);
}

bool SafeVisionaryData::isIntrudedPixelStateValid() const
{
  return (m_flags & INTRUDED_PIXEL_STATE_VALID_FLAG);
}

uint16_t SafeVisionaryData::getFlags() const
{
  return m_flags;
}

const ROI_DATA& SafeVisionaryData::getRoiData() const
{
  return m_roiData;
}

DEVICE_STATUS_ELEMENT SafeVisionaryData::getDeviceStatusData() const
{
  return m_deviceStatusData;
}

LOCALIOS_ELEMENT SafeVisionaryData::getLocalIOData() const
{
  return m_localIOsData;
}

const FIELDINFORMATION_DATA& SafeVisionaryData::getFieldInformationData() const
{
  return m_fieldInformationData;
}

const LOGICSIGNALS_DATA& SafeVisionaryData::getLogicSignalsData() const
{
  return m_logicSignalsData;
}

const IMU_ELEMENT SafeVisionaryData::getIMUData() const
{
  return m_IMUData;
}

DataHandlerError SafeVisionaryData::getLastError()
{
  return m_lastDataHandlerError;
}

DataSetsActive SafeVisionaryData::getDataSetsActive()
{
  return m_dataSetsActive;
}

void SafeVisionaryData::clearData(uint32_t changedCounter)
{
  if (!m_dataSetsActive.hasDataSetDepthMap)
  {
    m_distanceMap.clear();
    m_intensityMap.clear();
    m_stateMap.clear();

    // In case data segment "Depthmap" is not available use the changed counter as frame number.
    // The changed counter is incremented each Blob and is identical to the frame number.
    m_frameNum = changedCounter;
  }

  if (!m_dataSetsActive.hasDataSetDeviceStatus)
  {
    memset(&m_deviceStatusData, 0u, sizeof(m_deviceStatusData));
  }

  if (!m_dataSetsActive.hasDataSetROI)
  {
    memset(&m_roiData, 0u, sizeof(m_roiData));
  }

  if (!m_dataSetsActive.hasDataSetLocalIOs)
  {
    memset(&m_localIOsData, 0u, sizeof(m_localIOsData));
  }

  if (!m_dataSetsActive.hasDataSetFieldInfo)
  {
    memset(&m_fieldInformationData, 0u, sizeof(m_fieldInformationData));
  }

  if (!m_dataSetsActive.hasDataSetLogicSignals)
  {
    memset(&m_logicSignalsData, 0u, sizeof(m_logicSignalsData));
  }

  if (!m_dataSetsActive.hasDataSetIMU)
  {
    memset(&m_IMUData, 0u, sizeof(m_IMUData));
  }
}

} // namespace visionary
