//
// Copyright note: Redistribution and use in source, with or without modification, are permitted.
// 
// Created: August 2017
// 
// SICK AG, Waldkirch
// email: TechSupport0905@sick.de

#include "sick_safevisionary_base/VisionaryData.h"

#include <sstream>
#include <algorithm>
#include <cmath>
#include <cassert>
#include <limits>
#include <chrono>
#include <ctime>

namespace visionary 
{

const float bad_point = std::numeric_limits<float>::quiet_NaN();

VisionaryData::VisionaryData()
{
  m_frameNum = 0;
  m_cameraParams.width = 0;
  m_cameraParams.height = 0;
  m_preCalcCamInfoType = VisionaryData::UNKNOWN;
}

VisionaryData::~VisionaryData()
{
}

int VisionaryData::getItemLength(std::string dataType)
{
  //Transform String to lower case String
  std::transform(dataType.begin(), dataType.end(), dataType.begin(), ::tolower);

  if (dataType == "uint8")
  {
    return 1;
  }
  else if (dataType == "uint16")
  {
    return 2;
  }
  else if (dataType == "uint32")
  {
    return 4;
  }
  else if (dataType == "uint64")
  {
    return 8;
  }
  return 0;
}

void VisionaryData::preCalcCamInfo(const ImageType& imgType)
{
  assert(imgType != UNKNOWN);     // Unknown image type for the point cloud transformation
  
  m_preCalcCamInfo.reserve(m_cameraParams.height * m_cameraParams.width);

  //-----------------------------------------------
  // transform each pixel into Cartesian coordinates
  for (int row = 0; row < m_cameraParams.height; row++)
  {
    double yp = (m_cameraParams.cy - row) / m_cameraParams.fy;
    double yp2 = yp * yp;

    for (int col = 0; col < m_cameraParams.width; col++)
    {
        // we map from image coordinates with origin top left and x
        // horizontal (right) and y vertical
        // (downwards) to camera coordinates with origin in center and x
        // to the left and y upwards (seen
        // from the sensor position)
        const double xp = (m_cameraParams.cx - col) / m_cameraParams.fx;

        // correct the camera distortion
        const double r2 = xp * xp + yp2;
        const double r4 = r2 * r2;
        const double k = 1 + m_cameraParams.k1 * r2 + m_cameraParams.k2 * r4;
        
        // Undistorted direction vector of the point
        const float x = static_cast<float>(xp * k);
        const float y = static_cast<float>(yp * k);
        const float z = 1.0f;
        double s0 = 0;
        if (RADIAL == imgType)
        {
          s0 = std::sqrt(x * x + y * y + z * z) * 1000;
        }
        else if (PLANAR == imgType)
        {
          s0 = 1000;
        }
        else
        {
          assert(!"Unknown image type for the point cloud transformation");
        }
        PointXYZ point;
        point.x = static_cast<float>(x / s0);
        point.y = static_cast<float>(y / s0);
        point.z = static_cast<float>(z / s0);

        m_preCalcCamInfo.push_back(point);
    }
  }
  m_preCalcCamInfoType = imgType;
}

void VisionaryData::generatePointCloud(const std::vector<uint16_t>& map, const ImageType& imgType, std::vector<PointXYZ> &pointCloud)
{
  // Calculate disortion data from XML metadata once.
  if (m_preCalcCamInfoType != imgType)
  {
    preCalcCamInfo(imgType);
  }
  size_t cloudSize = map.size();
  pointCloud.resize(cloudSize);

  const float f2rc = static_cast<float>(m_cameraParams.f2rc / 1000.f); // PointCloud should be in [m] and not in [mm]

  const float pixelSizeZ = m_scaleZ;

  //-----------------------------------------------
  // transform each pixel into Cartesian coordinates
  std::vector<uint16_t>::const_iterator itMap = map.begin();
  std::vector<PointXYZ>::iterator itUndistorted = m_preCalcCamInfo.begin();
  std::vector<PointXYZ>::iterator itPC = pointCloud.begin();
  for (uint32_t i = 0; i < cloudSize; ++i, ++itPC, ++itMap, ++itUndistorted)
  //for (std::vector<PointXYZ>::iterator itPC = pointCloud.begin(), itEnd = pointCloud.end(); itPC != itEnd; ++itPC, ++itMap, ++itUndistorted)
  {
    PointXYZ point;
    // If point is valid put it to point cloud
    if (*itMap == 0 || *itMap == uint16_t(0xFFFF))
    {
      point.x = bad_point;
      point.y = bad_point;
      point.z = bad_point;
    }
    else
    {
      // calculate coordinates & store in point cloud vector
      float distance = static_cast<float>((*itMap)) * pixelSizeZ;
      point.x = itUndistorted->x * distance;
      point.y = itUndistorted->y * distance;
      point.z = itUndistorted->z * distance - f2rc;
    }
    *itPC = point;
  }
  return;
}

void VisionaryData::transformPointCloud(std::vector<PointXYZ> &pointCloud) const
{
  // turn cam 2 world translations from [m] to [mm]
  const double tx  = m_cameraParams.cam2worldMatrix[3]  / 1000.;
  const double ty  = m_cameraParams.cam2worldMatrix[7]  / 1000.;
  const double tz  = m_cameraParams.cam2worldMatrix[11] / 1000.;

  for (std::vector<PointXYZ>::iterator it = pointCloud.begin(), itEnd = pointCloud.end(); it != itEnd; ++it)
  {
    const double x = it->x;
    const double y = it->y;
    const double z = it->z;

    it->x = static_cast<float>(x * m_cameraParams.cam2worldMatrix[0] + y * m_cameraParams.cam2worldMatrix[1] + z * m_cameraParams.cam2worldMatrix[2] + tx);
    it->y = static_cast<float>(x * m_cameraParams.cam2worldMatrix[4] + y * m_cameraParams.cam2worldMatrix[5] + z * m_cameraParams.cam2worldMatrix[6] + ty);
    it->z = static_cast<float>(x * m_cameraParams.cam2worldMatrix[8] + y * m_cameraParams.cam2worldMatrix[9] + z * m_cameraParams.cam2worldMatrix[10] + tz);
  }
}

int VisionaryData::getHeight() const
{
  return m_cameraParams.height;
}

int VisionaryData::getWidth() const
{
  return m_cameraParams.width;
}

uint32_t VisionaryData::getFrameNum() const
{
  return m_frameNum;
}

uint64_t VisionaryData::getTimestamp() const
{
  return m_blobTimestamp;
}

uint64_t VisionaryData::getTimestampMS() const
{
  std::tm tm = {static_cast<int>((m_blobTimestamp & BITMASK_SECOND) >> 10),
               static_cast<int>((m_blobTimestamp & BITMASK_MINUTE) >> 16),
               static_cast<int>((m_blobTimestamp & BITMASK_HOUR) >> 22),
               static_cast<int>((m_blobTimestamp & BITMASK_DAY) >> 38),
               static_cast<int>(((m_blobTimestamp & BITMASK_MONTH) >> 43) - 1u),
               static_cast<int>(((m_blobTimestamp & BITMASK_YEAR) >> 47u) - 1900)};
  tm.tm_isdst = -1; // Use DST value from local time zone
  auto tp = std::chrono::system_clock::from_time_t(std::mktime(&tm));
  uint64_t timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(tp.time_since_epoch()).count()
                       + (m_blobTimestamp & BITMASK_MILLISECOND);

  return timestamp;
}

uint64_t VisionaryData::getSegmentTimestampMS(uint8_t segNum) const
{
  std::tm tm = {static_cast<int>((m_segmentTimestamp[segNum] & BITMASK_SECOND) >> 10),
               static_cast<int>((m_segmentTimestamp[segNum] & BITMASK_MINUTE) >> 16),
               static_cast<int>((m_segmentTimestamp[segNum] & BITMASK_HOUR) >> 22),
               static_cast<int>((m_segmentTimestamp[segNum] & BITMASK_DAY) >> 38),
               static_cast<int>(((m_segmentTimestamp[segNum] & BITMASK_MONTH) >> 43) - 1u),
               static_cast<int>(((m_segmentTimestamp[segNum] & BITMASK_YEAR) >> 47u) - 1900)};
  tm.tm_isdst = -1; // Use DST value from local time zone
  auto tp = std::chrono::system_clock::from_time_t(std::mktime(&tm));
  uint64_t segTimestamp = std::chrono::duration_cast<std::chrono::milliseconds>(tp.time_since_epoch()).count()
                       + (m_segmentTimestamp[segNum] & BITMASK_MILLISECOND);

  return segTimestamp;
}

const CameraParameters& VisionaryData::getCameraParameters() const
{
  return m_cameraParams;
}

}
