//
// Copyright note: Redistribution and use in source, with or without modification, are permitted.
// 
// Created: November 2019
// 
// SICK AG, Waldkirch
// email: TechSupport0905@sick.de

#include "CoLa2ProtocolHandler.h"
#include "VisionaryEndian.h"

namespace visionary 
{

CoLa2ProtocolHandler::CoLa2ProtocolHandler(ITransport& rTransport)
  : m_rTransport(rTransport)
  , m_ReqID(0)
  , m_sessionID(0)
{
}

CoLa2ProtocolHandler::~CoLa2ProtocolHandler()
{
}

bool CoLa2ProtocolHandler::openSession(uint8_t sessionTimeout /*secs*/)
{
  // TODO: request session id and open session
  std::vector<std::uint8_t> buffer = createCoLa2Header();
  buffer.push_back('O'); // Open Session
  buffer.push_back('x');
  buffer.push_back(sessionTimeout);  // sessionTimeout secs timeout

  // set length of client ID (flex string)
  buffer.insert(buffer.end(), { 0, 0 });
  *reinterpret_cast<uint16_t*>(&buffer[19]) = nativeToColaByteOrder<uint16_t>(2u);

  // set client ID
  buffer.insert(buffer.end(), {'E', 'x'}); // Example flexstring, max 32 bytes

  // Overwrite length
  *reinterpret_cast<uint32_t*>(&buffer[4]) = nativeToBigEndian(static_cast<uint32_t>(buffer.size()) - 8);

  //
  // send to socket
  //

  m_rTransport.send(buffer);
  buffer.clear();

  //
  // get response
  //

  m_rTransport.read(buffer, sizeof(uint32_t));
  // check for magic bytes
  const std::vector<uint8_t> MagicBytes = { 0x02, 0x02, 0x02, 0x02 };
  bool result = std::equal(MagicBytes.begin(), MagicBytes.end(), buffer.begin());
  if (result)
  {
    // get length
    m_rTransport.read(buffer, sizeof(uint32_t));
    const uint32_t length = readUnalignBigEndian<uint32_t>(buffer.data());
    m_rTransport.read(buffer, length);
  }
  else
  {
    // drop invalid data
    buffer.clear();
  }
  CoLaCommand response(buffer);

  m_sessionID = readUnalignBigEndian<uint32_t>(buffer.data() + 2);
  return true;
}

void CoLa2ProtocolHandler::closeSession()
{
  // TODO: close session
}

uint16_t CoLa2ProtocolHandler::getReqId()
{
  return ++m_ReqID;
}

std::vector<std::uint8_t> CoLa2ProtocolHandler::createCoLa2Header()
{
  std::vector<std::uint8_t> header;

  // insert magic bytes
  const uint8_t MAGIC_BYTE = 0x02;
  // inserts 8 bytes at front (Magic Bytes and length)
  for (uint8_t i = 0; i < 8; i++)
  {
    header.push_back(MAGIC_BYTE);
  }
  // add HubCntr
  header.push_back(0); // TBD

  // add NoC
  header.push_back(0); // TBD

  // add SockIdx0
  //header.insert(header.end(), { 0, 0, 0, 1 }); // TBD

  // add SessionID
  header.insert(header.end(), { 0, 0, 0, 0 });
  *reinterpret_cast<uint32_t*>(&header[10]) = nativeToBigEndian(static_cast<uint32_t>(m_sessionID));
  // add ReqID
  header.insert(header.end(), { 0, 0 });
  *reinterpret_cast<uint16_t*>(&header[14]) = nativeToBigEndian(static_cast<uint16_t>(getReqId()));

  return header;
}

CoLaCommand CoLa2ProtocolHandler::send(CoLaCommand cmd)
{
  //
  // convert cola cmd to vector buffer and add/fill header
  //

  std::vector<std::uint8_t> buffer;
  buffer = cmd.getBuffer();

  std::vector<std::uint8_t> header = createCoLa2Header();

  buffer.erase(buffer.begin()); // remove 's' from CoLaCommand buffer, not used in CoLa2
  buffer.insert(buffer.begin(), header.begin(), header.end());
  header.clear();

  // Overwrite length
  *reinterpret_cast<uint32_t*>(&buffer[4]) = nativeToBigEndian(static_cast<uint32_t>(buffer.size()) - 8);

  // Add checksum to end
  //buffer.insert(buffer.end(), calculateChecksum(buffer));

  //
  // send to socket
  //
  
  m_rTransport.send(buffer);
  buffer.clear();

  //
  // get response
  //
  
  m_rTransport.read(buffer, sizeof(uint32_t));
  // check for magic bytes
  const std::vector<uint8_t> MagicBytes = { 0x02, 0x02, 0x02, 0x02 };
  bool result = std::equal(MagicBytes.begin(), MagicBytes.end(), buffer.begin());
  if (result)
  {
    // get length
    m_rTransport.read(buffer, sizeof(uint32_t));
    const uint32_t length = readUnalignBigEndian<uint32_t>(buffer.data());
    m_rTransport.read(buffer, length);
  }
  else
  {
    // drop invalid data
    buffer.clear();
  }
  buffer.erase(buffer.begin(), buffer.begin() + 8); // drop header
  buffer.insert(buffer.begin(), 's'); // insert 's'
  CoLaCommand response(buffer);
  return response;
}

uint8_t CoLa2ProtocolHandler::calculateChecksum(const std::vector<uint8_t>& buffer)
{
  uint8_t checksum = 0;
  for (size_t i = 8; i < buffer.size(); i++)
  {
    checksum ^= buffer[i];
  }
  return checksum;
}

}
