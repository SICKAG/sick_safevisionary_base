//
// Copyright note: Redistribution and use in source, with or without modification, are permitted.
// 
// Created: October 2018
// 
// SICK AG, Waldkirch
// email: TechSupport0905@sick.de

#include "sick_safevisionary_base/PointCloudPlyWriter.h"
#include "sick_safevisionary_base/VisionaryEndian.h"

#include <iostream>
#include <fstream>

namespace visionary 
{

bool PointCloudPlyWriter::WriteFormatPLY(const char* filename, const std::vector<PointXYZ>& points, bool useBinary)
{
  return WriteFormatPLY(filename, points, std::vector<uint32_t>(), std::vector<uint16_t>(), useBinary);
}

bool PointCloudPlyWriter::WriteFormatPLY(const char* filename, const std::vector<PointXYZ>& points, const std::vector<uint32_t>& rgbaMap, bool useBinary)
{
  return WriteFormatPLY(filename, points, rgbaMap, std::vector<uint16_t>(), useBinary);
}

bool PointCloudPlyWriter::WriteFormatPLY(const char* filename, const std::vector<PointXYZ>& points, const std::vector<uint16_t>& intensityMap, bool useBinary)
{
  return WriteFormatPLY(filename, points, std::vector<uint32_t>(), intensityMap, useBinary);
}

bool PointCloudPlyWriter::WriteFormatPLY(const char* filename, const std::vector<PointXYZ>& points, const std::vector<uint32_t>& rgbaMap, const std::vector<uint16_t>& intensityMap, bool useBinary)
{
  bool success = true;

  bool hasColors = points.size() == rgbaMap.size();
  bool hasIntensities = points.size() == intensityMap.size();

  std::ofstream stream;

  // Open file
  stream.open(filename, useBinary ? (std::ios_base::out | std::ios_base::binary) : std::ios_base::out);

  if (stream.is_open())
  {
    // Write header
    stream << "ply\n";
    stream << "format " << (useBinary ? "binary_little_endian" : "ascii") << " 1.0\n";
    stream << "element vertex " << points.size() << "\n";
    stream << "property float x\n";
    stream << "property float y\n";
    stream << "property float z\n";
    if (hasColors)
    {
      stream << "property uchar red\n";
      stream << "property uchar green\n";
      stream << "property uchar blue\n";
    }
    if (hasIntensities)
    {
      stream << "property float intensity\n";
    }
    stream << "end_header\n";

    if (useBinary == false)
    {
      // Write all points
      for (int i = 0; i < static_cast<int>(points.size()); i++)
      {
        PointXYZ point = points.at(i);
        stream << point.x << " " << point.y << " " << point.z;

        if (hasColors)
        {
          const uint8_t *rgba = reinterpret_cast<const uint8_t*>(&rgbaMap.at(i));
          stream << " " << static_cast<uint32_t>(rgba[0]) << " " << static_cast<uint32_t>(rgba[1]) << " " << static_cast<uint32_t>(rgba[2]);
        }
        if (hasIntensities)
        {
          float intensity = static_cast<float>(intensityMap.at(i)) / 65535.0f;
          stream << " " << intensity;
        }

        stream << "\n";
      }
    }
    else
    {
      // Write all points
      for (int i = 0; i < static_cast<int>(points.size()); i++)
      {
        PointXYZ point = points.at(i);
        float x = nativeToLittleEndian(point.x);
        float y = nativeToLittleEndian(point.y);
        float z = nativeToLittleEndian(point.z);

        stream.write(reinterpret_cast<char*>(&x), 4);
        stream.write(reinterpret_cast<char*>(&y), 4);
        stream.write(reinterpret_cast<char*>(&z), 4);

        if (hasColors)
        {
          stream.write(reinterpret_cast<const char*>(&rgbaMap.at(i)), 3);
        }
        if (hasIntensities)
        {
          float intensity = static_cast<float>(intensityMap.at(i)) / 65535.0f;
          stream.write(reinterpret_cast<const char*>(&intensity), 4);
        }
      }
    }
  }
  else
  {
    success = false;
  }

  // Close file
  stream.close();

  return success;
}

PointCloudPlyWriter::PointCloudPlyWriter()
{
}

PointCloudPlyWriter::~PointCloudPlyWriter()
{
}

}
