//
// Copyright note: Redistribution and use in source, with or without modification, are permitted.
// 
// Created: August 2017
// 
// SICK AG, Waldkirch
// email: TechSupport0905@sick.de

#pragma once

#include <stdint.h>
#include <string>
#include <vector>
#include <iostream>

#include "PointXYZ.h"
#define TOTAL_SEGMENT_NUMBER	9

namespace visionary 
{

// Parameters to be extracted from the XML metadata part
struct CameraParameters {
  /// The height of the frame in pixels
  int height;
  /// The width of the frame in pixels
  int width;
  /// Camera to world transformation matrix
  double cam2worldMatrix[4 * 4];
  /// Camera Matrix
  double fx, fy, cx, cy;
  /// Camera Distortion Parameters
  double k1, k2, p1, p2, k3;
  /// FocalToRayCross - Correction Offset for depth info
  double f2rc;
};

struct DataSetsActive {
  bool hasDataSetDepthMap;
  bool hasDataSetPolar2D;
  bool hasDataSetCartesian;
  bool hasDataSetDeviceStatus;
  bool hasDataSetROI;
  bool hasDataSetLocalIOs;
  bool hasDataSetFieldInfo;
  bool hasDataSetLogicSignals;
  bool hasDataSetIMU;
};

struct PointXYZC {
  float x;
  float y;
  float z;
  float c;
};

class VisionaryData
{
public:
  VisionaryData();
  ~VisionaryData();

  //-----------------------------------------------
  // Getter Functions

  // Calculate and return the Point Cloud in the camera perspective. Units are in meters.
  virtual void generatePointCloud(std::vector<PointXYZ> &pointCloud) = 0;

  // Transform the XYZ point cloud with the Cam2World matrix got from device
  // IN/OUT pointCloud  - Reference to the point cloud to be transformed. Contains the transformed point cloud afterwards.
  void transformPointCloud(std::vector<PointXYZ> &pointCloud) const;

  int getHeight() const;

  int getWidth() const;
  // Returns the Byte length compared to data types

  uint32_t getFrameNum() const;
  // Returns the timestamp in device format
  // Bits of the devices timestamp: 5 unused - 12 Year - 4 Month - 5 Day - 11 Timezone - 5 Hour - 6 Minute - 6 Seconds - 10 Milliseconds
  // .....YYYYYYYYYYYYMMMMDDDDDTTTTTTTTTTTHHHHHMMMMMMSSSSSSmmmmmmmmmm
  uint64_t getTimestamp() const;
  // Returns the timestamp in milliseconds
  uint64_t getTimestampMS() const;
 // Return the time stamp in milliseconds for the specific segment
  uint64_t getSegmentTimestampMS(uint8_t segNum) const;

  // Returns a reference to the camera parameter struct
   // Returns a reference to the camera parameter struct
  const CameraParameters& getCameraParameters() const;

  //-----------------------------------------------
  // functions for parsing received blob
  
  // Parse the XML Metadata part to get information about the sensor and the following image data.
  // Returns true when parsing was successful.
  virtual bool parseXML(const std::string & xmlString, uint32_t changeCounter) = 0;

  // Parse the Binary data part to extract the image data. 
  // Returns true when parsing was successful.
  virtual bool parseBinaryData(std::vector<uint8_t>::iterator inputBuffer, size_t length) = 0;

  /// Parse the device status from the Blob segment "DepthMap".
   /// \return Returns true when parsing was successful.
  virtual bool parseDepthMap(std::vector<uint8_t>::iterator itBuf, size_t length){(void)(itBuf); (void)(length); return false;};

  /// Parse the ROI data from the Blob segment "ROI".
  /// \return Returns true when parsing was successful.
  virtual bool parseRoiData(std::vector<uint8_t>::iterator itBuf, size_t length){(void)(itBuf); (void)(length); return false;};

  /// Parse the Device status data from the Blob segment "Device Status"
  /// \return Returns true when parsing was successful.
  virtual bool parseDeviceStatusData(std::vector<uint8_t>::iterator itBuf, size_t length){(void)(itBuf); (void)(length); return false;};

  /// Parse the Local I/Os data from the Blob segment "Local I/Os"
  /// \return Returns true when parsing was successful.
  virtual bool parseLocalIOsData(std::vector<uint8_t>::iterator itBuf, size_t length){(void)(itBuf); (void)(length); return false;};

  /// Parse the Field Information data from the Blob segment "Field Information"
  /// \return Returns true when parsing was successful.
  virtual bool parseFieldInformationData(std::vector<uint8_t>::iterator itBuf, size_t length){(void)(itBuf); (void)(length); return false;};

  /// Parse the Logic Signals data from the Blob segment "Logic Signals"
  /// \return Returns true when parsing was successful.
  virtual bool parseLogicSignalsData(std::vector<uint8_t>::iterator itBuf, size_t length){(void)(itBuf); (void)(length); return false;};

  /// Parse the IMU data from the Blob segment "IMU"
  /// \return Returns true when parsing was successful.
  virtual bool parseIMUData(std::vector<uint8_t>::iterator itBuf, size_t length){(void)(itBuf); (void)(length); return false;};

  /// Clears the data from the last Blob in case the corresponding segment is not available any more.
  /// In case the data segment "DepthMap" is not available, use the given changed counter as framenumber.
  /// The changed counter is incremented each Blob and is identical to the frame number.
  ///
  /// \param[in] changedCounter counter which shall be used as frame number
  virtual void clearData(uint32_t changedCounter){(void)(changedCounter);};

  /// Gets the structure with the active segments.
  ///
  /// \return Returns the structure with the active segments
  virtual DataSetsActive getDataSetsActive(){return DataSetsActive();};

protected:
  // Device specific image types
  enum ImageType{UNKNOWN, PLANAR, RADIAL};

  // Returns the Byte length compared to data type given as String
  int getItemLength(std::string dataType);

  // Pre-calculate lookup table for lens distortion correction, 
  // which is needed for point cloud calculation.
  void preCalcCamInfo(const ImageType& type);

  // Calculate and return the Point Cloud in the camera perspective. Units are in meters.
  // IN  map         - Image to be transformed
  // IN  imgType     - Type of the image (needed for correct transformation)
  // OUT pointCloud  - Reference to pass back the point cloud. Will be resized and only contain new point cloud.
  void generatePointCloud(const std::vector<uint16_t>& map, const ImageType& imgType, std::vector<PointXYZ> &pointCloud);

  //-----------------------------------------------
  // Camera parameters to be read from XML Metadata part
  CameraParameters m_cameraParams;

  
  /// Factor to convert unit of distance image to mm
  float m_scaleZ;

  /// Change counter to detect changes in XML
  uint_fast32_t m_changeCounter;

  // Framenumber of the frame
  /// Dataset Version 1: incremented on each received image
  /// Dataset Version 2: framenumber received with dataset
  uint_fast32_t m_frameNum;

  // Timestamp in blob format
  // To get timestamp in milliseconds call getTimestampMS()
  uint64_t m_blobTimestamp;

  // Timestamp in blob format in each segment
  // To get timestamp in milliseconds call getTimestampMS()
  uint64_t m_segmentTimestamp[TOTAL_SEGMENT_NUMBER];

  // Camera undistort pre-calculations (look-up-tables) are generated to speed up computations. True if this has been done.
  ImageType m_preCalcCamInfoType;
  // The look-up-tables containing pre-calculations
  std::vector<PointXYZ> m_preCalcCamInfo;

private:
  // Bitmasks to calculate the timestamp in milliseconds
  // Bits of the devices timestamp: 5 unused - 12 Year - 4 Month - 5 Day - 11 Timezone - 5 Hour - 6 Minute - 6 Seconds - 10 Milliseconds
  // .....YYYYYYYYYYYYMMMMDDDDDTTTTTTTTTTTHHHHHMMMMMMSSSSSSmmmmmmmmmm
  static const uint64_t BITMASK_YEAR = 0x7FF800000000000; // 0000011111111111100000000000000000000000000000000000000000000000
  static const uint64_t BITMASK_MONTH = 0x780000000000;   // 0000000000000000011110000000000000000000000000000000000000000000
  static const uint64_t BITMASK_DAY = 0x7C000000000;      // 0000000000000000000001111100000000000000000000000000000000000000
  static const uint64_t BITMASK_HOUR = 0x7C00000;         // 0000000000000000000000000000000000000111110000000000000000000000
  static const uint64_t BITMASK_MINUTE = 0x3F0000;        // 0000000000000000000000000000000000000000001111110000000000000000
  static const uint64_t BITMASK_SECOND = 0xFC00;          // 0000000000000000000000000000000000000000000000001111110000000000
  static const uint64_t BITMASK_MILLISECOND = 0x3FF;      // 0000000000000000000000000000000000000000000000000000001111111111
};

}
