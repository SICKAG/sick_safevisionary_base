//
// Copyright note: Redistribution and use in source, with or without modification, are permitted.
//
// Created: November 2019
//
// SICK AG, Waldkirch
// email: TechSupport0905@sick.de

#include <cstring>
#include <stdio.h>

#include "sick_safevisionary_base/VisionaryDataStream.h"
#include "sick_safevisionary_base/VisionaryEndian.h"

namespace visionary {

VisionaryDataStream::VisionaryDataStream(std::shared_ptr<VisionaryData> dataHandler)
  : m_dataHandler(dataHandler)
{
}

VisionaryDataStream::~VisionaryDataStream() {}

bool VisionaryDataStream::open(const std::string& hostname, std::uint16_t port)
{
  m_pTransport = nullptr;

  std::unique_ptr<TcpSocket> pTransport(new TcpSocket());

  if (pTransport->connect(hostname, port) != 0)
  {
    return false;
  }

  m_pTransport = std::move(pTransport);

  return true;
}

void VisionaryDataStream::close()
{
  if (m_pTransport)
  {
    m_pTransport->shutdown();
    m_pTransport = nullptr;
  }
}

bool VisionaryDataStream::syncCoLa() const
{
  size_t elements = 0;
  std::vector<std::uint8_t> buffer;

  while (elements < 4)
  {
    if (m_pTransport->read(buffer, 1) < 1)
    {
      return false;
    }
    if (0x02 == buffer[0])
    {
      elements++;
    }
    else
    {
      elements = 0;
    }
  }

  return true;
}

bool VisionaryDataStream::getNextFrame()
{
  if (!syncCoLa())
  {
    return false;
  }

  std::vector<uint8_t> buffer;

  // Read package length
  if (m_pTransport->read(buffer, sizeof(uint32_t)) < (int)sizeof(uint32_t))
  {
    std::printf("Received less than the required 4 package length bytes.\n");
    return false;
  }

  const uint32_t packageLength = readUnalignBigEndian<uint32_t>(buffer.data());

  // Receive the frame data
  int remainingBytesToReceive = packageLength;
  m_pTransport->read(buffer, remainingBytesToReceive);

  // Check that protocol version and packet type are correct
  const uint16_t protocolVersion = readUnalignBigEndian<uint16_t>(buffer.data());
  const uint8_t packetType       = readUnalignBigEndian<uint8_t>(buffer.data() + 2);
  if (protocolVersion != 0x001)
  {
    std::printf("Received unknown protocol version %d.\n", protocolVersion);
    return false;
  }
  if (packetType != 0x62)
  {
    std::printf("Received unknown packet type %d\n.", packetType);
    return false;
  }

  return parseSegmentBinaryData(buffer.begin() + 3); // Skip protocolVersion and packetType
}

bool VisionaryDataStream::parseSegmentBinaryData(std::vector<uint8_t>::iterator itBuf)
{
  bool result                                 = false;
  std::vector<uint8_t>::iterator itBufSegment = itBuf;

  //-----------------------------------------------
  // Extract informations in Segment-Binary-Data
  // const uint16_t blobID = readUnalignBigEndian<uint16_t>(&*itBufSegment);
  itBufSegment += sizeof(uint16_t);
  const uint16_t numSegments = readUnalignBigEndian<uint16_t>(&*itBufSegment);
  itBufSegment += sizeof(uint16_t);

  // offset and changedCounter, 4 bytes each per segment
  std::vector<uint32_t> offset(numSegments);
  std::vector<uint32_t> changeCounter(numSegments);
  for (int i = 0; i < numSegments; i++)
  {
    offset[i] = readUnalignBigEndian<uint32_t>(&*itBufSegment);
    itBufSegment += sizeof(uint32_t);
    changeCounter[i] = readUnalignBigEndian<uint32_t>(&*itBufSegment);
    itBufSegment += sizeof(uint32_t);
  }

  //-----------------------------------------------
  // First segment contains the XML Metadata
  std::string xmlSegment((itBuf + offset[0]), (itBuf + offset[1]));
  if (m_dataHandler->parseXML(xmlSegment, changeCounter[0]))
  {
    //-----------------------------------------------
    // Second segment contains Binary data
    size_t binarySegmentSize = offset[2] - offset[1];
    result = m_dataHandler->parseBinaryData((itBuf + offset[1]), binarySegmentSize);
  }
  return result;
}

} // namespace visionary
