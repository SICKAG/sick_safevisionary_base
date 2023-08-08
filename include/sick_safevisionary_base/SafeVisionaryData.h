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

#include "VisionaryData.h"

#define DEPTHMAP_SEGMENT 1
#define DEVICESTATUS_SEGMENT 2
#define ROI_SEGMENT 3
#define LOCALIOS_SEGMENT 4
#define FIELDINFORMATION_SEGMENT 5
#define LOGICSIGNALS_SEGMENT 6
#define IMU_SEGMENT 7

namespace visionary {
/** Enumeration which describes the various states of the device */
enum class DEVICE_STATUS : std::uint8_t
{
  DEVICE_STATUS_CONFIGURATION       = 0U, /**< Device is in state CONFIGURATION */
  DEVICE_STATUS_WAIT_FOR_INPUTS     = 1U, /**< Device is in state WAIT_FOR_INPUTS */
  DEVICE_STATUS_APPLICATION_STOPPED = 2U, /**< Device is in state APPLICATION_STOPPED */
  DEVICE_STATUS_NORMAL_OPERATION =
    3U,                        /**< Device is in state NORMAL_OPERATION, application is running */
  DEVICE_STATUS_INVALID = 255U /**< Device status is invalid / not initialized */
};

/*
 * Segment 1  DepthMap
 */
#pragma pack(push, 1)

struct DEPTHMAP_FLAGS
{
  uint16_t filteredDepthMap : 1;
  uint16_t intrusionData : 1;
  uint16_t dataStream : 1;
  uint16_t reserved : 13;
};

struct DEPTHMAP_ELEMENT
{
  uint32_t frameNumber;
  DEVICE_STATUS deviceStatus;
  DEPTHMAP_FLAGS flags;
  uint16_t distance;
  uint16_t intensity;
  uint16_t confidence;
};

#pragma pack(pop)

/*
 * Segment 2  Device Status
 */
#pragma pack(push, 1)

struct DEVICESTATUS_DATA_GENERALSTATUS
{
  uint8_t runModeActive : 1;
  uint8_t deviceError : 1;
  uint8_t applicationError : 1;
  uint8_t sleepMode : 1;
  uint8_t waitForInput : 1;
  uint8_t waitForCluster : 1;
  uint8_t contaminationWarning : 1;
  uint8_t contaminationError : 1;
  uint8_t deadZoneDetection : 1;
  uint8_t temperatureWarning : 1;
  uint8_t reserved : 6;
};

struct DEVICESTATUS_DATA_ACTIVE_MONITORING_CASE
{
  uint8_t currentCaseNumberMonitoringCase1 : 8;
  uint8_t currentCaseNumberMonitoringCase2 : 8;
  uint8_t currentCaseNumberMonitoringCase3 : 8;
  uint8_t currentCaseNumberMonitoringCase4 : 8;
};

struct DEVICE_STATUS_ELEMENT
{
  DEVICESTATUS_DATA_GENERALSTATUS generalStatus;
  uint32_t COPSaftyRelated;    // Cut-Off-Path (safety related)
  uint32_t COPNonSaftyRelated; // Cut-Off-Path non safety related
  uint32_t COPResetRequired;   // Cut-Off-Path Reset Required
  DEVICESTATUS_DATA_ACTIVE_MONITORING_CASE activeMonitoringCase;
  uint8_t contaminationLevel;
};

#pragma pack(pop)

/*
 * Segment 3  ROI
 */
#pragma pack(push, 1)

struct ROI_DATA_RESULT
{
  uint8_t taskResult : 1;
  uint8_t resultSafe : 1;
  uint8_t resultValid : 1;
  uint8_t distanceValid : 1;
  uint8_t distanceSafe : 1;
  uint8_t reserved : 3;
};

/*! Quality classes for measurement accuracy (JAMA 3DTOF-BU_22_SYS-1316) */
enum ROI_QUALITY_CLASS
{

  ROI_QUALITY_CLASS_INVALID  = 0,
  ROI_QUALITY_CLASS_HIGH     = 1,
  ROI_QUALITY_CLASS_MODERATE = 2,
  ROI_QUALITY_CLASS_LOW      = 3

};

/*! \brief Union for BitSet structure and alternative generic access : ROI distance measurement -
 * safety related information (JAMA 3DTOF-BU_22_SYS-995) */
union ROI_DATA_SAFETY_DATA
{
  struct
  {
    uint8_t invalidDueToInvalidPixels : 1;
    uint8_t invalidDueToVariance : 1;
    uint8_t invalidDueToOverexposure : 1;
    uint8_t invalidDueToUnderexposure : 1;
    uint8_t invalidDueToTemporalVariance : 1;
    uint8_t invalidDueToOutsideOfMeasurementRange : 1;
    uint8_t invalidDueToRetroReflectorInterference : 1;
    uint8_t contaminationError : 1;
    uint8_t qualityClass : 2;
    uint8_t slotActive : 1;
    uint8_t reserved : 5;

  } tMembers; /*!< Member Container */

  uint16_t u16All;
};

struct ROI_ELEMENT
{
  uint8_t id;
  ROI_DATA_RESULT result;
  ROI_DATA_SAFETY_DATA safetyRelatedData;
  uint16_t distanceValue;
};

enum class DataHandlerError
{
  OK,
  PARSE_XML_ERROR,
  INVALID_CRC_SEGMENT_DEPTHMAP,
  INVALID_LENGTH_SEGMENT_DEPTHMAP,
  INVALID_VERSION_SEGMENT_DEPTHMAP,
  INVALID_CRC_SEGMENT_DEVICESTATUS,
  INVALID_LENGTH_SEGMENT_DEVICESTATUS,
  INVALID_VERSION_SEGMENT_DEVICESTATUS,
  INVALID_CRC_SEGMENT_ROI,
  INVALID_LENGTH_SEGMENT_ROI,
  INVALID_VERSION_SEGMENT_ROI,
  INVALID_CRC_SEGMENT_LOCALIOS,
  INVALID_LENGTH_SEGMENT_LOCALIOS,
  INVALID_VERSION_SEGMENT_LOCALIOS,
  INVALID_CRC_SEGMENT_FIELDINFORMATION,
  INVALID_LENGTH_SEGMENT_FIELDINFORMATION,
  INVALID_VERSION_SEGMENT_FIELDINFORMATION,
  INVALID_CRC_SEGMENT_LOGICSIGNALS,
  INVALID_LENGTH_SEGMENT_LOGICSIGNALS,
  INVALID_VERSION_SEGMENT_LOGICSIGNALS,
  INVALID_CRC_SEGMENT_IMU,
  INVALID_LENGTH_SEGMENT_IMU,
  INVALID_VERSION_SEGMENT_IMU
};

constexpr std::uint32_t MAX_ROI_VALUES = 5u;

struct ROI_DATA
{
  ROI_ELEMENT roiData[MAX_ROI_VALUES];
};
#pragma pack(pop)

/*
 * Segment 4  Local I/Os
 */
#pragma pack(push, 1)

struct LOCALIOS_UNIVERSALIO_CONFIGURED
{
  uint16_t configuredUniIOPin5 : 1;
  uint16_t configuredUniIOPin6 : 1;
  uint16_t configuredUniIOPin7 : 1;
  uint16_t configuredUniIOPin8 : 1;
  uint16_t reserved : 12;
};

struct LOCALIOS_UNIVERSALIO_DIRECTION
{
  uint16_t directionValueUniIOPin5 : 1;
  uint16_t directionValueUniIOPin6 : 1;
  uint16_t directionValueUniIOPin7 : 1;
  uint16_t directionValueUniIOPin8 : 1;
  uint16_t reserved : 12;
};

struct LOCALIOS_UNIVERSALIO_INPUTVALUES
{
  uint16_t logicalValueUniIOPin5 : 1;
  uint16_t logicalValueUniIOPin6 : 1;
  uint16_t logicalValueUniIOPin7 : 1;
  uint16_t logicalValueUniIOPin8 : 1;
  uint16_t reserved : 12;
};
struct LOCALIOS_UNIVERSALIO_OUTPUTVALUES
{
  uint8_t localOutput1Pin5;
  uint8_t localOutput2Pin6;
  uint8_t localOutput3Pin7;
  uint8_t localOutput4Pin8;
  uint8_t reserved[12]; // Byte 4 bis 15 are reserved
};

struct LOCALIOS_OSSDS_STATE
{
  uint8_t stateOSSD1A : 1;
  uint8_t stateOSSD1B : 1;
  uint8_t stateOSSD2A : 1;
  uint8_t stateOSSD2B : 1;
  uint8_t reserved : 4;
};

struct LOCALIOS_ELEMENT
{
  LOCALIOS_UNIVERSALIO_CONFIGURED universalIOConfigured;
  LOCALIOS_UNIVERSALIO_DIRECTION universalIODirection;
  LOCALIOS_UNIVERSALIO_INPUTVALUES universalIOInputValue;
  LOCALIOS_UNIVERSALIO_OUTPUTVALUES universalIOOutputValue;
  LOCALIOS_OSSDS_STATE ossdsState;
  uint8_t ossdsDynCount;      // reserved
  uint8_t ossdsCRC;           // reserved
  uint8_t ossdsIOStatus;      // reserved
  uint16_t dynamicSpeedA;     // reserved
  uint16_t dynamicSpeedB;     // reserved
  uint16_t DynamicValidFlags; // reserved
  uint16_t flags;             // reserved
};

#pragma pack(pop)

/*
 * Segment 5 Field Information
 */
#pragma pack(push, 1)

struct FIELDINFORMATION_ELEMENT
{
  uint8_t fieldID;
  uint8_t fieldSetID;
  uint8_t fieldResult;
  uint8_t evalMethod;
  uint8_t fieldActive;
};
constexpr std::uint32_t MAX_FIELDINFORMATION_VALUES = 16u;

struct FIELDINFORMATION_DATA
{
  FIELDINFORMATION_ELEMENT fieldInformation[MAX_FIELDINFORMATION_VALUES];
};
#pragma pack(pop)

/*
 * Segment 6 Logic Signals
 */
#pragma pack(push, 1)

struct LOGICSIGNALS_INSTANCESTATE
{
  uint8_t instanceOSSD1 : 1;
  uint8_t instanceOSSD2 : 1;
  uint8_t reserved : 6;
};

struct LOGICSIGNALS_ELEMENT
{
  uint8_t signalType;
  uint8_t instance;
  uint16_t configured : 1;
  uint16_t signalDirection : 1;
  uint16_t reserved : 14;
  uint16_t value;
};

constexpr std::uint32_t MAX_LOGICSIGNALS_VALUES = 20u;

struct LOGICSIGNALS_DATA
{
  LOGICSIGNALS_ELEMENT logicSignals[MAX_LOGICSIGNALS_VALUES];
};

#pragma pack(pop)

/*
 * Segment 7  IMU
 */
#pragma pack(push, 1)

struct IMU_VECTOR
{
  float X;
  float Y;
  float Z;
  uint8_t accuracy;
};

struct IMU_QUATERNION
{
  float X;
  float Y;
  float Z;
  float W;
  float accuracy;
};

struct IMU_ELEMENT
{
  IMU_VECTOR acceleration;
  IMU_VECTOR angularVelocity;
  IMU_QUATERNION orientation;
};

#pragma pack(pop)

class SafeVisionaryData : public VisionaryData
{
public:
  SafeVisionaryData();
  virtual ~SafeVisionaryData();

  //-----------------------------------------------
  // Getter Functions

  /// Gets the radial distance map
  /// The unit of the distance map is 1/4 mm.
  /// \return vector containing the radial distance map
  const std::vector<uint16_t>& getDistanceMap() const;

  /// Gets the intensity map
  /// \return vector containing the intensity map
  const std::vector<uint16_t>& getIntensityMap() const;

  /// Gets the pixel state map
  /// \return vector containing the pixel state map
  const std::vector<uint8_t>& getStateMap() const;

  /// Gets the flags state map
  /// \return flags state
  uint16_t getFlags() const;

  /// Gets device status
  /// \return received device status
  DEVICE_STATUS getDeviceStatus() const;

  /// Gets  Device Status element
  DEVICE_STATUS_ELEMENT getDeviceStatusData() const;

  /// Gets ROI data
  /// \return received ROI data
  const ROI_DATA& getRoiData() const;

  /// Gets Local I/Os
  /// \return received Local I/Os
  LOCALIOS_ELEMENT getLocalIOData() const;

  /// Gets Field Information
  /// \return received field Information
  const FIELDINFORMATION_DATA& getFieldInformationData() const;

  /// Gets Logic Signals data
  /// \return received Logic Signal data
  const LOGICSIGNALS_DATA& getLogicSignalsData() const;

  /// Gets IMU data
  /// \return received IMU data
  const IMU_ELEMENT getIMUData() const;

  /// Gets the information whether the distance map is filtered.
  /// \return returns true in case the distance map is filtered, otherwise returns false
  bool isDistanceMapFiltered() const;

  /// Gets the information whether the intruded pixel state in the pixel state map is valid.
  /// \return returns true in case the intruded pixel state is valid, otherwise returns false
  bool isIntrudedPixelStateValid() const;

  /// Calculate and return the point cloud in the camera perspective. Units are in meters.
  /// \param[out] vector containing the calculated point cloud
  void generatePointCloud(std::vector<PointXYZ>& pointCloud) override;

  /// factor to convert Radial distance map from fixed point to floating point
  static const float DISTANCE_MAP_UNIT;

  /// Gets the last error which occurred while parsing the Blob data segments.
  ///
  /// \return Returns the last error, OK in case there occurred no error
  DataHandlerError getLastError();

  /// Gets the structure with the active segments.
  ///
  /// \return Returns the structure with the active segments
  DataSetsActive getDataSetsActive();

  /// Clears the data from the last Blob in case the corresponding segment is not available any
  /// more. In case the data segment "DepthMap" is not available, use the given changed counter as
  /// framenumber. The changed counter is incremented each Blob and is identical to the frame
  /// number.
  ///
  /// \param[in] changedCounter counter which shall be used as frame number
  void clearData(uint32_t changedCounter);

protected:
  //-----------------------------------------------
  // functions for parsing received blob

  /// Parse the XML Metadata part to get information about the sensor and the following image data.
  /// \return Returns true when parsing was successful.
  bool parseXML(const std::string& xmlString, uint32_t changeCounter);

  /// Parse the Binary data part to extract the image data.
  /// some variables are commented out, because they are not used in this sample.
  /// \return Returns true when parsing was successful.
  bool parseBinaryData(std::vector<uint8_t>::iterator itBuf, size_t length);

  /// Parse the ROI data from the Blob segment "ROI".
  /// \return Returns true when parsing was successful.
  bool parseRoiData(std::vector<uint8_t>::iterator itBuf, size_t length);

  /// Parse the DeviceStatus data from the Blob segment "Device Status".
  /// \return Returns true when parsing was successful.
  bool parseDeviceStatusData(std::vector<uint8_t>::iterator itBuf, size_t length);

  /// Parse the Local I/Os data from the Blob segment "local I/Os".
  /// \return Returns true when parsing was successful.
  bool parseLocalIOsData(std::vector<uint8_t>::iterator itBuf, size_t length);

  /// Parse the field Information data from the Blob segment "Field Information ".
  /// \return Returns true when parsing was successful.
  bool parseFieldInformationData(std::vector<uint8_t>::iterator itBuf, size_t length);

  /// Parse the Logic Signals data from the Blob segment "Logic Signals".
  /// \return Returns true when parsing was successful.
  bool parseLogicSignalsData(std::vector<uint8_t>::iterator itBuf, size_t length);

  /// Parse the IMU data from the Blob segment "IMU".
  /// \return Returns true when parsing was successful.
  bool parseIMUData(std::vector<uint8_t>::iterator itBuf, size_t length);

private:
  // Indicator for the received data sets
  DataSetsActive m_dataSetsActive;

  /// Byte depth of depth map
  uint32_t m_distanceByteDepth;

  /// Byte depth of intensity map
  uint32_t m_intensityByteDepth;

  /// Byte depth of pixel state map
  uint32_t m_stateByteDepth;

  /// Vector containing the depth map
  std::vector<uint16_t> m_distanceMap;

  /// Vector containing the intensity map
  std::vector<uint16_t> m_intensityMap;

  /// Vector containing the pixel state map
  std::vector<uint8_t> m_stateMap;

  /// Contains the received device status
  DEVICE_STATUS m_deviceStatus;

  /// Contains the ROI data
  ROI_DATA m_roiData;

  // Contains the Device status data
  DEVICE_STATUS_ELEMENT m_deviceStatusData;

  // Contains the Local I/Os
  LOCALIOS_ELEMENT m_localIOsData;

  // Contains the Field Information data
  FIELDINFORMATION_DATA m_fieldInformationData;

  // Contains the Logic Signal data
  LOGICSIGNALS_DATA m_logicSignalsData;

  // Contains the IMU data
  IMU_ELEMENT m_IMUData;

  /// Contains the received flags
  uint16_t m_flags;

  /// Stores the last error which occurred while parsing the Blob data segments
  DataHandlerError m_lastDataHandlerError;
};

} // namespace visionary
