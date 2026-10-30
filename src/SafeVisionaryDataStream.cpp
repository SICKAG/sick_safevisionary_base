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

#include "sick_safevisionary_base/SafeVisionaryDataStream.h"
#include "sick_safevisionary_base/CRC.h"
#include "sick_safevisionary_base/VisionaryEndian.h"
#include <cstring>
#include <iostream>
#include <stdio.h>

//#define ENABLE_CRC_CHECK_UDP_FRAGMENT

namespace {
/// Maximum size of the BLOBs
/// todo
constexpr size_t BLOB_SIZE_MAX = 3000u * 1024u;

// Maximum IPv4 UDP payload. Newer safeVisionary streams use application datagrams around 64 KiB;
// the operating system reassembles their IPv4 fragments before returning one datagram from recv().
constexpr size_t MAX_UDP_BLOB_PACKET_SIZE = 65535u - 20u - 8u;

// Max TCP Packet size
// We don't support jumbo frames here
// The normal Ethernet MTU is 1500 bytes
// the minimum IPv4 header size is 20bytes
// the minimum IPv4 TCP header size is 20 bytes
constexpr size_t MAX_TCP_BLOB_PACKET_SIZE = 1500u - (20u + 20u);

/// Fixed value used in the protocols checksum field
/// A checksum is not necessary since error checking is done on lower protocol levels already
constexpr uint8_t PSEUDO_CHECKSUM = 0x45u;

// 4 byte 0x2020202
// 4 byte Packet Length
// 2 byte Protocol Version
// 1 byte Packet Type
constexpr int32_t BLOB_HEADER_SIZE = 11; // 4+4+2+1 = 11

#pragma pack(push, 1)
/// Structure of UDP header.
/// All values are big-endian.
struct UdpDataHeader
{
  uint16_t
    packetNumber; ///< packet number: current Blob number, incremented by one for each complete Blob
  uint16_t fragmentNumber; ///< current fragment number, incremented by one for new each fragment,
                           ///< set to 0 in case of a new Blob
  uint32_t timeStamp; ///< Time in us when the UDP datagram is generated, increasing monotonically
                      ///< starting from system initialization.
  uint32_t sourceIpAddress;  ///< IP address of the sensor
  uint16_t sourcePortNumber; ///< UDP port number of the sensor
  uint32_t destIpAddress;    ///< IP address of the target
  uint16_t destPortNumber;   ///< UDP port number of the target
  uint16_t protocolVersion;  ///< protocol version of the UDP header
  uint16_t
    dataLength; ///< length of the data within the UDP packet: bytes after protocol version
                ///< (including Packet Type) until end of fragment (including pseudo checksum)
  uint8_t flags; ///< flags of the fragment: Bit0-Bit6 reserved, Bit7 FIN: Set when this fragment is
                 ///< the last one of the Blob
  uint8_t packetType; //< type of the packed, set to 'b' = 0x62: Data packet
};

/// Structure of Blob header.
/// All values are big-endian.
struct BlobDataHeader
{
  uint32_t blobStart;  ///< 4 STx bytes, marks the start of Blob data, set to 0x02 0x02 0x02 0x02
  uint32_t blobLength; ///< length of the Blob data
  uint16_t protocolVersion;  ///< protocol version of the Blob data
  uint8_t packetType;        ///< type of the packet, set to 'b' = 0x62: Data packet
  uint16_t blobId;           ///< ID of the Blob data, set to 1 (3D data)
  uint16_t numberOfSegments; ///< number of data segments within the Blob data
};
#pragma pack(pop)

/// Version of UDP protocol
constexpr uint8_t UDP_PROTOCOL_VERSION = 0x0001u;

/// Packet type of UDP telegram: 'b' = Blob data
constexpr uint8_t PACKET_TYPE_DATA = 0x62u;

/// Bitmask for the flag last fragment
constexpr uint8_t FLAG_LAST_FRAGMENT = (1u << 7);

/// Blob data start bytes
constexpr uint32_t BLOB_DATA_START = 0x02020202u;

/// Protocol version of Blob data
constexpr uint8_t BLOB_DATA_PROTOCOL_VERSION = 0x0001u;

/// ID of Blob data (1: 3D data)
constexpr uint8_t BLOB_DATA_BLOB_ID = 0x0001u;

} // namespace

namespace visionary {
SafeVisionaryDataStream::SafeVisionaryDataStream(std::shared_ptr<VisionaryData> dataHandler)
  : m_dataHandler(dataHandler)
  , m_blobNumber(0u)
  , m_numSegments(0u)
  , m_lastDataStreamError(DataStreamError::OK)
{
  m_blobDataBuffer.reserve(
    BLOB_SIZE_MAX); // reserve maximum BLOB size to avoid (slow) reallocations
}

SafeVisionaryDataStream::~SafeVisionaryDataStream() {}

bool SafeVisionaryDataStream::openUdpConnection(std::uint16_t port)
{
  bool retValue{true};

  m_pTransportUdp = std::unique_ptr<UdpSocket>(new UdpSocket());
  if (m_pTransportUdp->bindPort(port) != 0)
  {
    m_pTransportUdp = nullptr;
    retValue        = false;
  }

  return retValue;
}

bool SafeVisionaryDataStream::openTcpConnection(std::uint16_t port, std::string deviceIpAddress)
{
  // TcpSocket::connect creates the client socket. The separate openTcp()
  // socket is only needed for the server-side API and must not be bound here.
  return m_pTransportTcp.connect(deviceIpAddress, port) == 0;
}
void SafeVisionaryDataStream::closeUdpConnection()
{
  if (m_pTransportUdp)
  {
    m_pTransportUdp->shutdown();
    m_pTransportUdp = nullptr;
  }
}
void SafeVisionaryDataStream::closeTcpConnection()
{
  // if (m_pTransportTcp)
  {
    m_pTransportTcp.shutdown();
    //  m_pTransportTcp = nullptr;
  }
}

bool SafeVisionaryDataStream::getNextFragment(std::vector<std::uint8_t>& receiveBuffer)
{
  int32_t receiveSize{0};

  // receive next UDP fragment
  receiveSize = m_pTransportUdp->recv(receiveBuffer, MAX_UDP_BLOB_PACKET_SIZE);

  if (receiveSize < 0)

  {
    // timeout
    std::printf("Blob data receive timeout\n");
    m_lastDataStreamError = DataStreamError::DATA_RECEIVE_TIMEOUT;
    return false;
  }

  if (0u == receiveSize)
  {
    // connection closed
    std::printf("Blob connection closed\n");
    m_lastDataStreamError = DataStreamError::CONNECTION_CLOSED;
    return false;
  }

  receiveBuffer.resize(receiveSize);

  return true;
}

int32_t SafeVisionaryDataStream::getNextTcpReception(std::vector<std::uint8_t>& receiveBuffer)
{
  int32_t receiveSize{0};

  // receive next TCP packet
  receiveSize = m_pTransportTcp.recv(receiveBuffer, MAX_TCP_BLOB_PACKET_SIZE);

  if (receiveSize < 0)
  {
    std::printf("Receive Failed\n");
    m_lastDataStreamError = DataStreamError::DATA_RECEIVE_TIMEOUT;
    return -1;
  }

  if (0u == receiveSize)
  {
    // connection closed
    std::printf("Connection closed\n");
    m_lastDataStreamError = DataStreamError::CONNECTION_CLOSED;
    return -1;
  }

  if (receiveSize > 0)
    receiveBuffer.resize(receiveSize);

  return receiveSize;
}

bool SafeVisionaryDataStream::parseUdpHeader(std::vector<std::uint8_t>& buffer,
                                             UdpProtocolData& udpProtocolData)
{
  udpProtocolData = {0u, 0u, 0u, false};

  constexpr size_t UDP_TRAILER_SIZE = sizeof(uint32_t);
  if (buffer.size() < sizeof(UdpDataHeader) + UDP_TRAILER_SIZE)
  {
    std::printf("Received UDP packet is too short for the protocol header and trailer.\n");
    m_lastDataStreamError = DataStreamError::INVALID_LENGTH_UDP_HEADER;
    return false;
  }

  // read UPD data header
  UdpDataHeader* pUdpHeader = reinterpret_cast<UdpDataHeader*>(buffer.data());

  const uint16_t protocolVersion = readUnalignBigEndian<uint16_t>(&pUdpHeader->protocolVersion);
  if (protocolVersion != UDP_PROTOCOL_VERSION)
  {
    // unknown protocol version
    std::printf("Received unknown protocol version of UDP header: %d.\n", protocolVersion);
    m_lastDataStreamError = DataStreamError::INVALID_VERSION_UDP_HEADER;
    return false;
  }

#ifdef ENABLE_CRC_CHECK_UDP_FRAGMENT
  const uint32_t udpDataSize =
    static_cast<uint16_t>(buffer.size()) - static_cast<uint16_t>(sizeof(uint32_t));
  const uint32_t crc32 = readUnalignBigEndian<uint32_t>(buffer.data() + udpDataSize);

  const uint32_t crc32Calculated =
    ~CRC_calcCrc32CBlock(buffer.data(), udpDataSize, CRC_DEFAULT_INIT_VALUE32);

  if (crc32 != crc32Calculated)
  {
    std::printf("Malformed data, CRC32C checksum does not match.\n");
    m_lastDataStreamError = DataStreamError::INVALID_CRC_UDP_HEADER;
    return false;
  }
#endif

  if (pUdpHeader->packetType != PACKET_TYPE_DATA)
  {
    // unexpected packet type
    std::printf("Received unknown packet type: %d\n.", pUdpHeader->packetType);
    m_lastDataStreamError = DataStreamError::INVALID_PACKET_TYPE_UDP_HEADER;
    return false;
  }

  // check length of received packet
  const uint16_t fragmentLength = readUnalignBigEndian<uint16_t>(&pUdpHeader->dataLength);
  // received length =length of received datagram - size of UDP header - size of CRC value
  const size_t receivedLength = buffer.size() - sizeof(UdpDataHeader) - UDP_TRAILER_SIZE;
  if (fragmentLength != receivedLength)
  {
    // packet truncated, invalid
    std::printf("Received unexpected packet length. Expected length: %u, Received length: %zu\n.",
                static_cast<unsigned>(fragmentLength),
                receivedLength);
    m_lastDataStreamError = DataStreamError::INVALID_LENGTH_UDP_HEADER;
    return false;
  }

  // store relevant UDP protocol data for later use

  udpProtocolData.blobNumber     = readUnalignBigEndian<uint16_t>(&pUdpHeader->packetNumber);
  udpProtocolData.fragmentNumber = readUnalignBigEndian<uint16_t>(&pUdpHeader->fragmentNumber);
  udpProtocolData.dataLength     = fragmentLength;
  udpProtocolData.isLastFragment = (0u != (pUdpHeader->flags & FLAG_LAST_FRAGMENT));

  return true;
}

bool SafeVisionaryDataStream::getBlobStartUdp(bool& lastFragment)
{
  std::vector<uint8_t> receiveBuffer;
  bool foundBlobStart{false};
  lastFragment = false;

  while (!foundBlobStart)
  {
    // receive next UDP fragment
    if (!getNextFragment(receiveBuffer))
    {
      // getting the next UDP fragment failed, e.g. the UPD connection timed out
      break;
    }

    // parse and check UDP data header
    UdpProtocolData udpProtocolData{};
    if (!parseUdpHeader(receiveBuffer, udpProtocolData))
    {
      // UDP protocol error, e.g. wrong version, unexpected length
      break;
    }

    // check whether we have the first fragment of a new Blob
    if (udpProtocolData.fragmentNumber == 0U)
    {
      // copy payload of first fragment into buffer
      const auto payloadBegin = receiveBuffer.begin() + sizeof(UdpDataHeader);
      m_blobDataBuffer.assign(payloadBegin, payloadBegin + udpProtocolData.dataLength);
      m_blobNumber = udpProtocolData.blobNumber;
      if (udpProtocolData.isLastFragment)
      {
        // Blob data consists only of one UDP fragment
        lastFragment = true;
      }

      // found valid start of Blob data
      foundBlobStart = true;
    }

    // not first fragment -> continue with next UDP fragment
  }
  return foundBlobStart;
}

bool SafeVisionaryDataStream::getBlobStartTcp(std::vector<std::uint8_t>& receiveBufferPacketSize)
{
  int32_t receiveSize = 0;
  int blobCounter     = 0;

  while (true)
  {
    receiveSize = getNextTcpReception(receiveBufferPacketSize);

    if (receiveSize == BLOB_HEADER_SIZE)
    {
      blobCounter += 1;
    }

    // throw the first packet and receive from the second packet
    if (receiveSize == BLOB_HEADER_SIZE && blobCounter == 2)
    {
      // we have the header of second Blob -> check Blob protocol header
      BlobDataHeader* pBlobHeader =
        reinterpret_cast<BlobDataHeader*>(receiveBufferPacketSize.data());
      // check Blob data start bytes
      const uint32_t blobDataStartBytes = readUnalignBigEndian<uint32_t>(&pBlobHeader->blobStart);
      if (blobDataStartBytes == BLOB_DATA_START)
      {
        // std::cout<<"Receiving Blob is started."<<std::endl;
        return true; // start bytes of Blob data have been found
      }
      else
      {
        blobCounter = 0; // start bytes of Blob data have not been found. Try again
      }
    }
  }

  return false;
}

bool SafeVisionaryDataStream::parseBlobHeaderTcp()
{
  bool blobHeaderValid{true};

  // we have the first fragment -> check Blob protocol header
  BlobDataHeader* pBlobHeader = reinterpret_cast<BlobDataHeader*>(m_blobDataBuffer.data());

  //  check Blob data start bytes
  const uint32_t blobDataStartBytes = readUnalignBigEndian<uint32_t>(&pBlobHeader->blobStart);
  if (blobDataStartBytes != BLOB_DATA_START)
  {
    // start bytes of Blob data have not been found
    std::printf("Received unknown Blob data start bytes: %d.\n", blobDataStartBytes);
    m_lastDataStreamError = DataStreamError::INVALID_BLOB_HEADER;
    blobHeaderValid       = false;
  }

  // Check protocol version of Blob data
  const uint16_t protocolVersion = readUnalignBigEndian<uint16_t>(&pBlobHeader->protocolVersion);
  if (protocolVersion != BLOB_DATA_PROTOCOL_VERSION)
  {
    std::printf("Received unknown protocol version: %d.\n", protocolVersion);
    m_lastDataStreamError = DataStreamError::INVALID_VERSION_BLOB_HEADER;
    blobHeaderValid       = false;
  }

  // Check packet type of Blob data
  const uint8_t packetType = pBlobHeader->packetType;
  if (packetType != PACKET_TYPE_DATA)
  {
    std::printf("Received unknown packet type: %d\n.", packetType);
    m_lastDataStreamError = DataStreamError::INVALID_PACKET_TYPE_BLOB_HEADER;
    blobHeaderValid       = false;
  }

  // Check Blob ID of Blob data
  const uint16_t blobId = readUnalignBigEndian<uint16_t>(&pBlobHeader->blobId);

  if (blobId != BLOB_DATA_BLOB_ID)
  {
    std::printf("Received unknown Blob ID: %d\n.", blobId);
    m_lastDataStreamError = DataStreamError::INVALID_BLOB_ID;
    blobHeaderValid       = false;
  }

  if (blobHeaderValid)
  {
    // get number of Blob data segments
    m_numSegments = readUnalignBigEndian<uint16_t>(&pBlobHeader->numberOfSegments);
    std::cout << '\n' << "Number of Segments: " << m_numSegments << "	";

    m_offsetSegment.clear();
    m_changeCounter.clear();

    uint32_t blobDataPos = sizeof(BlobDataHeader);
    for (uint32_t segmentCounter = 0u; segmentCounter < m_numSegments; segmentCounter++)
    {
      // get offset of each Blob data segment within the complete Blob data
      m_offsetSegment.push_back(readUnalignBigEndian<uint32_t>(&m_blobDataBuffer[blobDataPos]));
      blobDataPos += sizeof(uint32_t);

      // get change counter of each Blob data segment
      m_changeCounter.push_back(readUnalignBigEndian<uint32_t>(&m_blobDataBuffer[blobDataPos]));
      blobDataPos += sizeof(uint32_t);
    }

    // add additional offset to be able to calculate the length of the last data segment by
    // calculating the difference of the last two offsets
    //@todo Why -3 Bytes? Is BlobLenght correct?
    m_offsetSegment.push_back(readUnalignBigEndian<uint32_t>(&pBlobHeader->blobLength) - 3u);
  }

  return blobHeaderValid;
}

bool SafeVisionaryDataStream::parseBlobHeaderUdp()
{
  if (m_blobDataBuffer.size() < sizeof(BlobDataHeader))
  {
    std::printf("Received Blob is too short for its header.\n");
    m_lastDataStreamError = DataStreamError::INVALID_BLOB_HEADER;
    return false;
  }

  bool blobHeaderValid{true};

  // The complete Blob has been reassembled before this function is called.
  BlobDataHeader* pBlobHeader = reinterpret_cast<BlobDataHeader*>(m_blobDataBuffer.data());

  // check Blob data start bytes
  const uint32_t blobDataStartBytes = readUnalignBigEndian<uint32_t>(&pBlobHeader->blobStart);
  if (blobDataStartBytes != BLOB_DATA_START)
  {
    // start bytes of Blob data have not been found
    std::printf("Received unknown Blob data start bytes: %d.\n", blobDataStartBytes);
    m_lastDataStreamError = DataStreamError::INVALID_BLOB_HEADER;
    blobHeaderValid       = false;
  }
  //  std::printf("Packet Size: %d\n", pBlobHeader->blobLength);
  // Check protocol version of Blob data
  const uint16_t protocolVersion = readUnalignBigEndian<uint16_t>(&pBlobHeader->protocolVersion);
  if (protocolVersion != BLOB_DATA_PROTOCOL_VERSION)
  {
    std::printf("Received unknown protocol version: %d.\n", protocolVersion);
    m_lastDataStreamError = DataStreamError::INVALID_VERSION_BLOB_HEADER;
    blobHeaderValid       = false;
  }

  // Check packet type of Blob data
  const uint8_t packetType = pBlobHeader->packetType;
  if (packetType != PACKET_TYPE_DATA)
  {
    std::printf("Received unknown packet type: %d\n.", packetType);
    m_lastDataStreamError = DataStreamError::INVALID_PACKET_TYPE_BLOB_HEADER;
    blobHeaderValid       = false;
  }

  // Check Blob ID of Blob data
  const uint16_t blobId = readUnalignBigEndian<uint16_t>(&pBlobHeader->blobId);

  if (blobId != BLOB_DATA_BLOB_ID)
  {
    std::printf("Received unknown Blob ID: %d\n.", blobId);
    m_lastDataStreamError = DataStreamError::INVALID_BLOB_ID;
    blobHeaderValid       = false;
  }

  const uint32_t blobLength = readUnalignBigEndian<uint32_t>(&pBlobHeader->blobLength);
  const size_t completeBlobSize = static_cast<size_t>(blobLength) + 2u * sizeof(uint32_t);
  if (completeBlobSize != m_blobDataBuffer.size())
  {
    std::printf("Received Blob length does not match its header.\n");
    m_lastDataStreamError = DataStreamError::INVALID_BLOB_HEADER;
    blobHeaderValid       = false;
  }

  if (blobHeaderValid)
  {
    // get number of Blob data segments
    m_numSegments = readUnalignBigEndian<uint16_t>(&pBlobHeader->numberOfSegments);
    // std::cout << '\n' << "Receiving segments: " << m_numSegments << "	 ";

    m_offsetSegment.clear();
    m_changeCounter.clear();

    const size_t segmentTableSize = static_cast<size_t>(m_numSegments) * 2u * sizeof(uint32_t);
    if (m_numSegments == 0u || sizeof(BlobDataHeader) + segmentTableSize > m_blobDataBuffer.size())
    {
      std::printf("Received invalid Blob segment table size.\n");
      m_lastDataStreamError = DataStreamError::INVALID_BLOB_HEADER;
      return false;
    }

    size_t blobDataPos = sizeof(BlobDataHeader);
    for (uint32_t segmentCounter = 0u; segmentCounter < m_numSegments; segmentCounter++)
    {
      // get offset of each Blob data segment within the complete Blob data
      m_offsetSegment.push_back(readUnalignBigEndian<uint32_t>(&m_blobDataBuffer[blobDataPos]));
      blobDataPos += sizeof(uint32_t);

      // get change counter of each Blob data segment
      m_changeCounter.push_back(readUnalignBigEndian<uint32_t>(&m_blobDataBuffer[blobDataPos]));
      blobDataPos += sizeof(uint32_t);
    }

    // Segment offsets are relative to byte 11 of the Blob. blobLength excludes the initial
    // start marker and length fields (8 bytes), hence the final relative offset is
    // (blobLength + 8) - 11.
    constexpr size_t SEGMENT_OFFSET_BASE = sizeof(BlobDataHeader) - 2u * sizeof(uint16_t);
    const uint32_t finalOffset = static_cast<uint32_t>(m_blobDataBuffer.size() - SEGMENT_OFFSET_BASE);
    m_offsetSegment.push_back(finalOffset);

    const uint32_t minimumFirstOffset = static_cast<uint32_t>(
      sizeof(BlobDataHeader) + segmentTableSize - SEGMENT_OFFSET_BASE);
    if (m_offsetSegment.front() < minimumFirstOffset)
    {
      std::printf("First Blob segment overlaps the segment table.\n");
      m_lastDataStreamError = DataStreamError::INVALID_BLOB_HEADER;
      return false;
    }

    for (size_t segmentCounter = 0u; segmentCounter < m_numSegments; ++segmentCounter)
    {
      if (m_offsetSegment[segmentCounter] > m_offsetSegment[segmentCounter + 1u] ||
          m_offsetSegment[segmentCounter + 1u] > finalOffset)
      {
        std::printf("Received invalid or non-monotonic Blob segment offsets.\n");
        m_lastDataStreamError = DataStreamError::INVALID_BLOB_HEADER;
        return false;
      }
    }
  }

  return blobHeaderValid;
}

bool SafeVisionaryDataStream::parseBlobData()
{
  uint32_t currentSegment{0};

  // First segment always contains the XML Metadata
  // Blob data begins after packet type, so subtract length of blobId and numberOfSegments
  uint32_t beginOfBlobData = sizeof(BlobDataHeader) - 2 * sizeof(uint16_t);
  std::string xmlSegment(&m_blobDataBuffer[beginOfBlobData + m_offsetSegment[currentSegment]],
                         &m_blobDataBuffer[beginOfBlobData + m_offsetSegment[currentSegment + 1]]);

  if (m_dataHandler->parseXML(xmlSegment, m_changeCounter[currentSegment]))
  {
    auto dataSetsActive = m_dataHandler->getDataSetsActive();
    if (dataSetsActive.hasDataSetDepthMap)
    {
      currentSegment++;
      // next segment contains the image data
      size_t binarySegmentSize =
        m_offsetSegment[currentSegment + 1] - m_offsetSegment[currentSegment];
      auto beginOfDataSegmentDepthMapIter =
        m_blobDataBuffer.begin() + beginOfBlobData + m_offsetSegment[currentSegment];
      if (!m_dataHandler->parseBinaryData(beginOfDataSegmentDepthMapIter, binarySegmentSize))
      {
        m_lastDataStreamError = DataStreamError::DATA_SEGMENT_DEPTHMAP_ERROR;
        return false;
      }
    }

    if (dataSetsActive.hasDataSetDeviceStatus)
    {
      currentSegment++;
      // next segment contains the device status
      size_t segmentSizeDeviceStatus =
        m_offsetSegment[currentSegment + 1] - m_offsetSegment[currentSegment];
      auto beginOfDataSegmentDeviceStatusIter =
        m_blobDataBuffer.begin() + beginOfBlobData + m_offsetSegment[currentSegment];

      if (!m_dataHandler->parseDeviceStatusData(beginOfDataSegmentDeviceStatusIter,
                                                segmentSizeDeviceStatus))
      {
        m_lastDataStreamError = DataStreamError::DATA_SEGMENT_DEVICESTATUS_ERROR;
        return false;
      }
    }

    if (dataSetsActive.hasDataSetROI)
    {
      currentSegment++;
      // next segment contains the ROI data
      size_t segmentSizeROI = m_offsetSegment[currentSegment + 1] - m_offsetSegment[currentSegment];
      auto beginOfDataSegmentRoiIter =
        m_blobDataBuffer.begin() + beginOfBlobData + m_offsetSegment[currentSegment];
      if (!m_dataHandler->parseRoiData(beginOfDataSegmentRoiIter, segmentSizeROI))
      {
        m_lastDataStreamError = DataStreamError::DATA_SEGMENT_ROI_ERROR;
        return false;
      }
    }

    if (dataSetsActive.hasDataSetLocalIOs)
    {
      currentSegment++;
      // next segment contains the Local I/Os
      size_t segmentSizeLocalIOs =
        m_offsetSegment[currentSegment + 1] - m_offsetSegment[currentSegment];
      auto beginOfDataSegmentLocalIOsIter =
        m_blobDataBuffer.begin() + beginOfBlobData + m_offsetSegment[currentSegment];
      if (!m_dataHandler->parseLocalIOsData(beginOfDataSegmentLocalIOsIter, segmentSizeLocalIOs))
      {
        m_lastDataStreamError = DataStreamError::DATA_SEGMENT_LOCALIOS_ERROR;
        return false;
      }
    }
    if (dataSetsActive.hasDataSetFieldInfo)
    {
      currentSegment++;
      // next segment contains the Field Information
      size_t segmentSizeFieldInformation =
        m_offsetSegment[currentSegment + 1] - m_offsetSegment[currentSegment];
      auto beginOfDataSegmentFieldInformationIter =
        m_blobDataBuffer.begin() + beginOfBlobData + m_offsetSegment[currentSegment];

      if (!m_dataHandler->parseFieldInformationData(beginOfDataSegmentFieldInformationIter,
                                                    segmentSizeFieldInformation))
      {
        m_lastDataStreamError = DataStreamError::DATA_SEGMENT_FIELDINFORMATION_ERROR;
        return false;
      }
    }
    if (dataSetsActive.hasDataSetLogicSignals)
    {
      currentSegment++;
      // next segment contains the logic signals
      size_t segmentSizeLogicSignals =
        m_offsetSegment[currentSegment + 1] - m_offsetSegment[currentSegment];
      auto beginOfDataSegmentLogicSignalsIter =
        m_blobDataBuffer.begin() + beginOfBlobData + m_offsetSegment[currentSegment];
      if (!m_dataHandler->parseLogicSignalsData(beginOfDataSegmentLogicSignalsIter,
                                                segmentSizeLogicSignals))
      {
        m_lastDataStreamError = DataStreamError::DATA_SEGMENT_LOGICSIGNALS_ERROR;
        return false;
      }
    }
    if (dataSetsActive.hasDataSetSafety)
    {
      currentSegment++;
      // A potential safety-segment parsing callback would be called here.
    }
    if (dataSetsActive.hasDataSetIMU)
    {
      currentSegment++;
      // next segment contains the IMU
      size_t segmentSizeIMU = m_offsetSegment[currentSegment + 1] - m_offsetSegment[currentSegment];
      auto beginOfDataSegmentIMUIter =
        m_blobDataBuffer.begin() + beginOfBlobData + m_offsetSegment[currentSegment];
      if (!m_dataHandler->parseIMUData(beginOfDataSegmentIMUIter, segmentSizeIMU))
      {
        m_lastDataStreamError = DataStreamError::DATA_SEGMENT_IMU_ERROR;
        return false;
      }
    }

    if (currentSegment + 1u != m_numSegments)
    {
      std::printf("Blob segment count does not match the datasets described by XML.\n");
      m_lastDataStreamError = DataStreamError::INVALID_BLOB_HEADER;
      return false;
    }
  }
  else
  {
    m_lastDataStreamError = DataStreamError::PARSE_XML_ERROR;
    return false;
  }
  // in case data segment "Depthmap" is not available use the changed counter of segment 1 as frame
  // number the changed counter is incremented each Blob and is identical to the frame number
  // segment 1 is always available, so use the changed counter of this segment
  m_dataHandler->clearData(m_changeCounter[m_changeCounter.size() > 1u ? 1u : 0u]);
  return true;
}

bool SafeVisionaryDataStream::getNextBlobUdp()
{
  std::vector<uint8_t> receiveBuffer;
  uint16_t expectedFragmentNumber{0u};
  bool blobDataComplete{false};
  bool lastFragment{false};

  m_blobDataBuffer.clear();

  if (getBlobStartUdp(lastFragment))
  {
    // in case the Blob data consists only of one fragment, nothing has to be done here, the
    // following loop must not be entered
    blobDataComplete = lastFragment;
    while (!blobDataComplete)
    {
      expectedFragmentNumber++;

      UdpProtocolData udpProtocolData{};
      // receive next UDP fragment
      if (!getNextFragment(receiveBuffer))
      {
        break;
      }

      // parse and check UDP data header
      if (!parseUdpHeader(receiveBuffer, udpProtocolData))
      {
        // UDP protocol error, e.g. wrong version, unexpected length
        break;
      }

      if (m_blobNumber != udpProtocolData.blobNumber)
      {
        //@todo Remove error message and start to receive a new Blob instead?
        // a new Blob has started (and we missed the rest of the BLOB we were just reading)
        std::printf("Unexpected Blob Number: expected value: %d, received value: %d\n",
                    m_blobNumber,
                    udpProtocolData.blobNumber);
        m_lastDataStreamError = DataStreamError::INVALID_BLOB_NUMBER;
        break;
      }

      if (expectedFragmentNumber != udpProtocolData.fragmentNumber)
      {
        //@todo Remove error message and start to receive a new Blob instead?
        // expected frame number does not match, we probably lost one fragment
        std::printf("Unexpected fragment number: expected value: %d, received value: %d\n",
                    expectedFragmentNumber,
                    udpProtocolData.fragmentNumber);
        m_lastDataStreamError = DataStreamError::INVALID_UDP_FRAGMENT_NUMBER;
        break;
      }

      if (m_blobDataBuffer.size() + udpProtocolData.dataLength > BLOB_SIZE_MAX)
      {
        std::printf("Received Blob exceeds the supported maximum size.\n");
        m_lastDataStreamError = DataStreamError::INVALID_BLOB_HEADER;
        break;
      }

      // append payload of new fragment to the Blob data
      const auto payloadBegin = receiveBuffer.begin() + sizeof(UdpDataHeader);
      m_blobDataBuffer.insert(
        m_blobDataBuffer.end(), payloadBegin, payloadBegin + udpProtocolData.dataLength);

      if (udpProtocolData.isLastFragment)
      {
        blobDataComplete = true;
      }
    }
  }

  bool result{false};
  if (blobDataComplete && parseBlobHeaderUdp())
  {
    result = parseBlobData();
    if (result)
    {
      m_lastDataStreamError = DataStreamError::OK;
    }
  }

  return result;
}

bool SafeVisionaryDataStream::getNextBlobTcp(std::vector<std::uint8_t>& receiveBufferPacketSize)
{
  receiveBufferPacketSize.clear();
  // TCP is a byte stream; recv() boundaries cannot be used as frame
  // boundaries. The SICK Visionary stream uses STX, a big-endian payload
  // length, then the payload.
  std::vector<uint8_t> byte;
  byte.reserve(1u);
  std::size_t stxBytes{0u};
  while (stxBytes < sizeof(uint32_t))
  {
    if (m_pTransportTcp.read(byte, 1u) != 1)
    {
      m_lastDataStreamError = DataStreamError::DATA_RECEIVE_TIMEOUT;
      return false;
    }

    if (byte[0] == 0x02u)
    {
      ++stxBytes;
    }
    else
    {
      stxBytes = 0u;
    }
  }

  std::vector<uint8_t> lengthBuffer;
  if (m_pTransportTcp.read(lengthBuffer, sizeof(uint32_t)) != sizeof(uint32_t))
  {
    m_lastDataStreamError = DataStreamError::DATA_RECEIVE_TIMEOUT;
    return false;
  }

  const uint32_t payloadLength = readUnalignBigEndian<uint32_t>(lengthBuffer.data());
  constexpr uint32_t MIN_PAYLOAD_LENGTH = sizeof(BlobDataHeader) - 2u * sizeof(uint32_t);
  if (payloadLength < MIN_PAYLOAD_LENGTH || payloadLength > BLOB_SIZE_MAX - 2u * sizeof(uint32_t))
  {
    std::printf("Received invalid TCP Blob length: %u.\n", payloadLength);
    m_lastDataStreamError = DataStreamError::INVALID_BLOB_HEADER;
    return false;
  }

  std::vector<uint8_t> payload;
  if (m_pTransportTcp.read(payload, payloadLength) != static_cast<int>(payloadLength))
  {
    m_lastDataStreamError = DataStreamError::DATA_RECEIVE_TIMEOUT;
    return false;
  }

  // Keep the complete wire representation. parseBlobHeaderUdp() validates
  // the common length-prefixed blob format and prepares the segment table;
  // the validation itself is independent of the transport.
  m_blobDataBuffer.resize(2u * sizeof(uint32_t) + payloadLength);
  std::memset(m_blobDataBuffer.data(), 0x02, sizeof(uint32_t));
  std::memcpy(m_blobDataBuffer.data() + sizeof(uint32_t), lengthBuffer.data(), sizeof(uint32_t));
  std::memcpy(m_blobDataBuffer.data() + 2u * sizeof(uint32_t), payload.data(), payload.size());

  if (!parseBlobHeaderUdp())
  {
    return false;
  }

  const bool result = parseBlobData();
  if (result)
  {
    m_lastDataStreamError = DataStreamError::OK;
  }
  return result;
}

DataStreamError SafeVisionaryDataStream::getLastError()
{
  return m_lastDataStreamError;
}

} // namespace visionary
