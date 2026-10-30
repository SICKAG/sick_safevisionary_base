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

#include <cstdio>

#include "sick_safevisionary_base/CRC.h"
#include "sick_safevisionary_base/SafeVisionaryData.h"
#include "sick_safevisionary_base/VisionaryEndian.h"
#include <algorithm>
#include <cstring>
#include <iostream>
#include <limits>
// TinyXML-2 XML DOM parser
#include "sick_safevisionary_base/tinyxml2.h"

namespace visionary {

constexpr float SafeVisionaryData::DISTANCE_MAP_UNIT = 0.25f;

namespace {
/// Version number of the Blob segment DepthMap
constexpr std::uint16_t VERSION_SEGMENT_DEPTHMAP = 0x0002u;

/// Version number of the Blob segment ROI
constexpr std::uint16_t VERSION_SEGMENT_DEVICESTATUS_V1 = 0x0001u;
constexpr std::uint16_t VERSION_SEGMENT_DEVICESTATUS_V2 = 0x0002u;

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

/** Flag whether the ghost filter was active for the depth map. */
constexpr std::uint16_t GHOST_FILTER_ACTIVE_FLAG = 1u << 2;

} // namespace
SafeVisionaryData::SafeVisionaryData()
  : VisionaryData()
  , m_dataSetsActive()
  , m_distanceByteDepth(0u)
  , m_intensityByteDepth(0u)
  , m_stateByteDepth(0u)
  , m_deviceStatus(DEVICE_STATUS::DEVICE_STATUS_INVALID)
  , m_expectedRoiCount(0u)
  , m_flags(0u)
  , m_hasConfigurationChecksum(false)
  , m_lastDataHandlerError(DataHandlerError::OK)
{
  m_changeCounter = std::numeric_limits<uint_fast32_t>::max();
  std::fill(m_segmentTimestamp, m_segmentTimestamp + TOTAL_SEGMENT_NUMBER, 0u);
  std::memset(&m_roiData, 0, sizeof(m_roiData));
  std::memset(&m_deviceStatusData, 0, sizeof(m_deviceStatusData));
  std::memset(&m_localIOsData, 0, sizeof(m_localIOsData));
  std::memset(&m_fieldInformationData, 0, sizeof(m_fieldInformationData));
  std::memset(&m_logicSignalsData, 0, sizeof(m_logicSignalsData));
  std::memset(&m_IMUData, 0, sizeof(m_IMUData));
}

SafeVisionaryData::~SafeVisionaryData() {}

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
  auto tXMLError = xmlTree.Parse(xmlString.c_str());
  if (tXMLError != tinyxml2::XMLError::XML_SUCCESS)
  {
    std::printf("Reading XML tree in BLOB failed.");
    m_lastDataHandlerError = DataHandlerError::PARSE_XML_ERROR;
    return false;
  }

  //-----------------------------------------------
  // Check whether datasets entry exists and whether it has a depthmap
  bool oParseOk                           = true;
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

      ptHasNodeOrZero = ptDataSetsTree->FirstChildElement("DataSetDeviceStatus");
      m_dataSetsActive.hasDataSetDeviceStatus = (ptHasNodeOrZero != 0);

      ptHasNodeOrZero                = ptDataSetsTree->FirstChildElement("DataSetROI");
      m_dataSetsActive.hasDataSetROI = (ptHasNodeOrZero != 0);
      m_expectedRoiCount             = 0u;
      if (ptHasNodeOrZero != 0)
      {
        auto const* formatDescription = ptHasNodeOrZero->FirstChildElement("FormatDescriptionROI");
        auto const* measurementTask =
          formatDescription != 0 ? formatDescription->FirstChildElement("MeasurementTask") : 0;
        while (measurementTask != 0)
        {
          ++m_expectedRoiCount;
          measurementTask = measurementTask->NextSiblingElement("MeasurementTask");
        }
      }

      ptHasNodeOrZero                     = ptDataSetsTree->FirstChildElement("DataSetLocalIOs");
      m_dataSetsActive.hasDataSetLocalIOs = (ptHasNodeOrZero != 0);

      ptHasNodeOrZero = ptDataSetsTree->FirstChildElement("DataSetFieldInformation");
      m_dataSetsActive.hasDataSetFieldInfo = (ptHasNodeOrZero != 0);

      ptHasNodeOrZero = ptDataSetsTree->FirstChildElement("DataSetLogicalSignals");
      m_dataSetsActive.hasDataSetLogicSignals = (ptHasNodeOrZero != 0);

      ptHasNodeOrZero                  = ptDataSetsTree->FirstChildElement("DataSetSafety");
      m_dataSetsActive.hasDataSetSafety = (ptHasNodeOrZero != 0);

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
      auto const* ptCameraToWorldTransform =
        ptDataStreamTree->FirstChildElement("CameraToWorldTransform");
      if (ptCameraToWorldTransform != 0)
      {
        auto const* ptMatrixEntry = ptCameraToWorldTransform->FirstChildElement();
        for (auto idx = 0; idx < 4 * 4; ++idx)
        {
          tXMLError     = ptMatrixEntry->QueryDoubleText(&m_cameraParams.cam2worldMatrix[idx]);
          ptMatrixEntry = ptMatrixEntry->NextSiblingElement();
          if ((tXMLError != tinyxml2::XMLError::XML_SUCCESS) or
              ((idx < 4 * 4 - 1) and (ptMatrixEntry == 0)))
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

    // const auto distanceDecimalExponent =
    // dataStreamTree.get<int>("Distance.<xmlattr>.decimalexponent", 0); Scaling is fixed to 0.25mm
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
    std::printf(
      "Malformed data, length in data segment depth map header does not match package size.\n");
    m_lastDataHandlerError = DataHandlerError::INVALID_LENGTH_SEGMENT_DEPTHMAP;
    return false;
  }

  // get second length field
  const uint32_t lengthCopy = readUnalignLittleEndian<uint32_t>(&*(itBuf + dataSize + 4));

  // check whether length matches
  if (length != lengthCopy)
  {
    std::printf("Malformed data, length of data segment depth map header does not match data "
                "segment size.\n");
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
  constexpr size_t SEGMENT_PREFIX_SIZE = sizeof(uint64_t) + sizeof(uint16_t);
  constexpr size_t ROI_ELEMENT_SIZE = sizeof(uint8_t) + sizeof(uint8_t) + sizeof(uint16_t) +
                                      sizeof(uint16_t);

  if (size < sizeof(uint32_t))
  {
    m_lastDataHandlerError = DataHandlerError::INVALID_LENGTH_SEGMENT_ROI;
    return false;
  }

  // get length of data segment ROI
  const uint32_t length = readUnalignLittleEndian<uint32_t>(&*itBuf);
  itBuf += sizeof(uint32_t);

  // length does not include the second length field, so add 4 bytes
  if (length < 8u + SEGMENT_PREFIX_SIZE || (static_cast<size_t>(length) + sizeof(uint32_t)) != size)
  {
    std::printf("Malformed data, length of data segment ROI does not match package size.\n");
    m_lastDataHandlerError = DataHandlerError::INVALID_LENGTH_SEGMENT_ROI;
    return false;
  }

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

  const size_t roiPayloadSize = dataSize - SEGMENT_PREFIX_SIZE;
  if ((roiPayloadSize % ROI_ELEMENT_SIZE) != 0u)
  {
    std::printf("Malformed data, ROI payload is not a sequence of ROI elements.\n");
    m_lastDataHandlerError = DataHandlerError::INVALID_LENGTH_SEGMENT_ROI;
    return false;
  }

  const size_t roiCount = roiPayloadSize / ROI_ELEMENT_SIZE;
  if (m_expectedRoiCount != 0u && roiCount != m_expectedRoiCount)
  {
    std::printf("Malformed data, ROI element count does not match the XML metadata.\n");
    m_lastDataHandlerError = DataHandlerError::INVALID_LENGTH_SEGMENT_ROI;
    return false;
  }
  m_roiDataElements.clear();
  m_roiDataElements.reserve(roiCount);
  std::memset(&m_roiData, 0, sizeof(m_roiData));

  for (size_t roiIndex = 0; roiIndex < roiCount; ++roiIndex)
  {
    ROI_ELEMENT roi{};
    roi.id = readUnalignLittleEndian<uint8_t>(&*itBuf);
    itBuf += sizeof(uint8_t);

    const uint8_t result = readUnalignLittleEndian<uint8_t>(&*itBuf);
    itBuf += sizeof(uint8_t);
    roi.result.taskResult    = (result >> 0u) & 1u;
    roi.result.resultSafe    = (result >> 1u) & 1u;
    roi.result.resultValid   = (result >> 2u) & 1u;
    roi.result.distanceValid = (result >> 3u) & 1u;
    roi.result.distanceSafe  = (result >> 4u) & 1u;

    roi.safetyRelatedData.u16All = readUnalignLittleEndian<uint16_t>(&*itBuf);
    itBuf += sizeof(uint16_t);
    roi.distanceValue = readUnalignLittleEndian<uint16_t>(&*itBuf);
    itBuf += sizeof(uint16_t);

    m_roiDataElements.push_back(roi);
    if (roiIndex < MAX_ROI_VALUES)
    {
      m_roiData.roiData[roiIndex] = roi;
    }
  }

  return true;
}

bool SafeVisionaryData::parseDeviceStatusData(std::vector<uint8_t>::iterator itBuf, size_t size)
{
  constexpr size_t SEGMENT_PREFIX_SIZE = sizeof(uint64_t) + sizeof(uint16_t);
  constexpr size_t DEVICE_STATUS_V1_PAYLOAD_SIZE = 2u + 3u * sizeof(uint32_t) + 4u + 1u;
  constexpr size_t DEVICE_STATUS_V2_PAYLOAD_SIZE =
    DEVICE_STATUS_V1_PAYLOAD_SIZE + sizeof(uint32_t);

  if (size < sizeof(uint32_t))
  {
    m_lastDataHandlerError = DataHandlerError::INVALID_LENGTH_SEGMENT_DEVICESTATUS;
    return false;
  }

  // get length of data segment Device Status
  const uint32_t length = readUnalignLittleEndian<uint32_t>(&*itBuf);
  itBuf += sizeof(uint32_t);

  // length does not include the second length field, so add 4 bytes
  if (length < 8u + SEGMENT_PREFIX_SIZE || (static_cast<size_t>(length) + sizeof(uint32_t)) != size)
  {
    std::printf("Malformed data, length of Device Status header does not match package size.\n");
    m_lastDataHandlerError = DataHandlerError::INVALID_LENGTH_SEGMENT_DEVICESTATUS;
    return false;
  }

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

  const size_t payloadSize = dataSize - SEGMENT_PREFIX_SIZE;
  const bool isVersion1 = version == VERSION_SEGMENT_DEVICESTATUS_V1;
  const bool isVersion2 = version == VERSION_SEGMENT_DEVICESTATUS_V2;
  if (!isVersion1 && !isVersion2)
  {
    std::printf("Unsupported version of data segment Device Status\n");
    m_lastDataHandlerError = DataHandlerError::INVALID_VERSION_SEGMENT_DEVICESTATUS;
    return false;
  }

  const size_t expectedPayloadSize =
    isVersion2 ? DEVICE_STATUS_V2_PAYLOAD_SIZE : DEVICE_STATUS_V1_PAYLOAD_SIZE;
  if (payloadSize != expectedPayloadSize)
  {
    std::printf("Malformed data, Device Status payload size does not match its version.\n");
    m_lastDataHandlerError = DataHandlerError::INVALID_LENGTH_SEGMENT_DEVICESTATUS;
    return false;
  }

  std::memset(&m_deviceStatusData, 0, sizeof(m_deviceStatusData));
  const uint16_t generalStatus = readUnalignLittleEndian<uint16_t>(&*itBuf);
  itBuf += sizeof(uint16_t);
  m_deviceStatusData.generalStatus.runModeActive          = (generalStatus >> 0u) & 1u;
  m_deviceStatusData.generalStatus.deviceError            = (generalStatus >> 1u) & 1u;
  m_deviceStatusData.generalStatus.applicationError       = (generalStatus >> 2u) & 1u;
  m_deviceStatusData.generalStatus.sleepMode              = (generalStatus >> 3u) & 1u;
  m_deviceStatusData.generalStatus.waitForInput           = (generalStatus >> 4u) & 1u;
  m_deviceStatusData.generalStatus.waitForCluster         = (generalStatus >> 5u) & 1u;
  m_deviceStatusData.generalStatus.contaminationWarning   = (generalStatus >> 6u) & 1u;
  m_deviceStatusData.generalStatus.contaminationError     = (generalStatus >> 7u) & 1u;
  m_deviceStatusData.generalStatus.deadZoneDetection      = (generalStatus >> 8u) & 1u;
  m_deviceStatusData.generalStatus.temperatureWarning     = (generalStatus >> 9u) & 1u;
  m_deviceStatusData.generalStatus.deadZoneWarning        = (generalStatus >> 10u) & 1u;

  m_deviceStatusData.COPSaftyRelated = readUnalignLittleEndian<uint32_t>(&*itBuf);
  itBuf += sizeof(uint32_t);
  m_deviceStatusData.COPNonSaftyRelated = readUnalignLittleEndian<uint32_t>(&*itBuf);
  itBuf += sizeof(uint32_t);
  m_deviceStatusData.COPResetRequired = readUnalignLittleEndian<uint32_t>(&*itBuf);
  itBuf += sizeof(uint32_t);

  m_deviceStatusData.activeMonitoringCase.currentCaseNumberMonitoringCase1 = *itBuf++;
  m_deviceStatusData.activeMonitoringCase.currentCaseNumberMonitoringCase2 = *itBuf++;
  m_deviceStatusData.activeMonitoringCase.currentCaseNumberMonitoringCase3 = *itBuf++;
  m_deviceStatusData.activeMonitoringCase.currentCaseNumberMonitoringCase4 = *itBuf++;
  m_deviceStatusData.contaminationLevel = *itBuf++;

  m_hasConfigurationChecksum = isVersion2;
  if (isVersion2)
  {
    m_deviceStatusData.configurationChecksum = readUnalignLittleEndian<uint32_t>(&*itBuf);
  }

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
    std::printf(
      "Malformed data, CRC32 checksum of data segment Field Information does not match.\n");
    m_lastDataHandlerError = DataHandlerError::INVALID_CRC_SEGMENT_FIELDINFORMATION;
    return false;
  }

  // length does not include the second length field, so add 4 bytes
  if ((length + sizeof(uint32_t)) != size)
  {
    std::printf(
      "Malformed data, length of data segment Field Information does not match package size.\n");
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
    std::printf(
      "Malformed data, length of data segment Logic Signals does not match package size.\n");
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

  m_blobTimestamp                 = readUnalignLittleEndian<uint64_t>(&*itBuf);
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

bool SafeVisionaryData::isGhostFilterActive() const
{
  return (m_flags & GHOST_FILTER_ACTIVE_FLAG);
}

uint16_t SafeVisionaryData::getFlags() const
{
  return m_flags;
}

const ROI_DATA& SafeVisionaryData::getRoiData() const
{
  return m_roiData;
}

const std::vector<ROI_ELEMENT>& SafeVisionaryData::getRoiDataElements() const
{
  return m_roiDataElements;
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

bool SafeVisionaryData::hasConfigurationChecksum() const
{
  return m_hasConfigurationChecksum;
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
    m_hasConfigurationChecksum = false;
  }

  if (!m_dataSetsActive.hasDataSetROI)
  {
    memset(&m_roiData, 0u, sizeof(m_roiData));
    m_roiDataElements.clear();
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
