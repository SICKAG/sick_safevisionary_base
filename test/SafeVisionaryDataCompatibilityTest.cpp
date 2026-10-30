// -- BEGIN LICENSE BLOCK ----------------------------------------------
/*!
* Copyright (C) 2026, SICK AG, Waldkirch, Germany
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

#include "sick_safevisionary_base/CRC.h"
#include "sick_safevisionary_base/SafeVisionaryData.h"

#include <cassert>
#include <cstdint>
#include <vector>

namespace {

class TestSafeVisionaryData : public visionary::SafeVisionaryData
{
public:
  bool parseDeviceStatus(std::vector<uint8_t>& segment)
  {
    return parseDeviceStatusData(segment.begin(), segment.size());
  }

  bool parseRoi(std::vector<uint8_t>& segment)
  {
    return parseRoiData(segment.begin(), segment.size());
  }

};

template <typename T>
void appendLittleEndian(std::vector<uint8_t>& buffer, T value)
{
  for (size_t byte = 0; byte < sizeof(T); ++byte)
  {
    buffer.push_back(static_cast<uint8_t>((value >> (8u * byte)) & 0xffu));
  }
}

std::vector<uint8_t> makeSegment(uint16_t version, const std::vector<uint8_t>& payload)
{
  std::vector<uint8_t> body;
  appendLittleEndian<uint64_t>(body, 0x123456789abcdef0ULL);
  appendLittleEndian<uint16_t>(body, version);
  body.insert(body.end(), payload.begin(), payload.end());

  const uint32_t length = static_cast<uint32_t>(body.size() + 8u);
  const uint32_t crc =
    ~visionary::CRC_calcCrc32Block(
      body.data(), static_cast<uint32_t>(body.size()), CRC_DEFAULT_INIT_VALUE32);

  std::vector<uint8_t> segment;
  appendLittleEndian<uint32_t>(segment, length);
  segment.insert(segment.end(), body.begin(), body.end());
  appendLittleEndian<uint32_t>(segment, crc);
  appendLittleEndian<uint32_t>(segment, length);
  return segment;
}

std::vector<uint8_t> makeDeviceStatusPayload(bool version2)
{
  std::vector<uint8_t> payload;
  appendLittleEndian<uint16_t>(payload, 0x0701u); // run mode, bits 8, 9 and 10
  appendLittleEndian<uint32_t>(payload, 0x11223344u);
  appendLittleEndian<uint32_t>(payload, 0x55667788u);
  appendLittleEndian<uint32_t>(payload, 0x99aabbccu);
  payload.push_back(1u);
  payload.push_back(2u);
  payload.push_back(3u);
  payload.push_back(4u);
  payload.push_back(75u);
  if (version2)
  {
    appendLittleEndian<uint32_t>(payload, 0xdeadbeefu);
  }
  return payload;
}

void testDeviceStatusVersions()
{
  TestSafeVisionaryData data;

  auto version1 = makeSegment(1u, makeDeviceStatusPayload(false));
  assert(data.parseDeviceStatus(version1));
  assert(!data.hasConfigurationChecksum());
  assert(data.getDeviceStatusData().configurationChecksum == 0u);
  assert(data.getDeviceStatusData().generalStatus.deadZoneWarning == 1u);

  auto version2 = makeSegment(2u, makeDeviceStatusPayload(true));
  assert(data.parseDeviceStatus(version2));
  assert(data.hasConfigurationChecksum());
  assert(data.getDeviceStatusData().configurationChecksum == 0xdeadbeefu);
  assert(data.getDeviceStatusData().COPSaftyRelated == 0x11223344u);
  assert(data.getDeviceStatusData().activeMonitoringCase.currentCaseNumberMonitoringCase4 == 4u);
}

void testUnsupportedDeviceStatusVersion()
{
  TestSafeVisionaryData data;
  auto unsupported = makeSegment(3u, makeDeviceStatusPayload(false));
  assert(!data.parseDeviceStatus(unsupported));
  assert(data.getLastError() == visionary::DataHandlerError::INVALID_VERSION_SEGMENT_DEVICESTATUS);
}

void testDynamicRoiCount()
{
  std::vector<uint8_t> payload;
  for (uint8_t index = 0; index < 8u; ++index)
  {
    payload.push_back(index);
    payload.push_back(0x1fu);
    appendLittleEndian<uint16_t>(payload, static_cast<uint16_t>(0x0300u + index));
    appendLittleEndian<uint16_t>(payload, static_cast<uint16_t>(100u + index));
  }

  TestSafeVisionaryData data;
  auto segment = makeSegment(1u, payload);
  assert(data.parseRoi(segment));
  assert(data.getRoiDataElements().size() == 8u);
  assert(data.getRoiDataElements()[7].id == 7u);
  assert(data.getRoiDataElements()[7].distanceValue == 107u);
  assert(data.getRoiData().roiData[4].id == 4u); // legacy five-entry view remains populated
}


} // namespace

int main()
{
  testDeviceStatusVersions();
  testUnsupportedDeviceStatusVersion();
  testDynamicRoiCount();
  return 0;
}
