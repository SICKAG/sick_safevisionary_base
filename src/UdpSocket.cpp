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

#include <cstring>

#include "sick_safevisionary_base/UdpSocket.h"

namespace visionary {

UdpSocket::UdpSocket()
  : m_socket()
{
  memset(&m_udpAddr, 0, sizeof(m_udpAddr));
}

int UdpSocket::initSocket()
{
  int iResult         = 0;
  long timeoutSeconds = 5L;

#ifdef _WIN32
  //-----------------------------------------------
  // Initialize Winsock
  WSADATA wsaData;
  iResult = ::WSAStartup(MAKEWORD(2, 2), &wsaData);
  if (iResult != NO_ERROR)
  {
    return iResult;
  }
#endif

  //-----------------------------------------------
  // Create a receiver socket to receive datagrams
  m_socket = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if (m_socket == INVALID_SOCKET)
  {
    return static_cast<int>(INVALID_SOCKET);
  }

  // Set the timeout for the socket to 5 seconds
#ifdef _WIN32
  // On Windows timeout is a DWORD in milliseconds
  // (https://docs.microsoft.com/en-us/windows/desktop/api/winsock/nf-winsock-setsockopt)
  long timeoutMs = timeoutSeconds * 1000L;
  iResult =
    setsockopt(m_socket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeoutMs, sizeof(timeoutMs));
#else
  struct timeval tv;
  tv.tv_sec  = timeoutSeconds; /* 5 seconds Timeout */
  tv.tv_usec = 0L;             // Not init'ing this can cause strange errors
  iResult = setsockopt(m_socket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(struct timeval));
#endif

  return iResult;
}

int UdpSocket::connect(const std::string& hostname, uint16_t port)
{
  int iResult = 0;
  int trueVal = 1;

  iResult = initSocket();

  // Enable UDP broadcast
  if (iResult >= 0)
  {
    //-----------------------------------------------
    // Bind the socket to any address and the specified port.
    m_udpAddr.sin_family      = AF_INET;
    m_udpAddr.sin_port        = port;
    m_udpAddr.sin_addr.s_addr = inet_addr(hostname.c_str());
    iResult =
      setsockopt(m_socket, SOL_SOCKET, SO_BROADCAST, (const char*)&trueVal, sizeof(trueVal));
  }

  return iResult;
}

int UdpSocket::bindPort(std::uint16_t port)
{
  int iResult = 0;

  iResult = initSocket();

  if (iResult >= 0)
  {
    // set socket receive buffer size to 512kB
    int bufferSize    = 512 * 1024;
    int bufferSizeLen = sizeof(bufferSize);
    iResult = setsockopt(m_socket, SOL_SOCKET, SO_RCVBUF, (char*)&bufferSize, bufferSizeLen);

    if (iResult >= 0)
    {
      //-----------------------------------------------
      // Bind socket to given port
      sockaddr_in saddr;
      saddr.sin_family      = AF_INET;
      saddr.sin_addr.s_addr = htonl(INADDR_ANY);
      saddr.sin_port        = port;
      iResult               = bind(m_socket, reinterpret_cast<sockaddr*>(&saddr), sizeof(saddr));
    }
  }
  return iResult;
}

int UdpSocket::shutdown()
{
  // Close the socket when finished receiving datagrams
#ifdef _WIN32
  closesocket(m_socket);
  WSACleanup();
#else
  close(m_socket);
#endif
  m_socket = static_cast<int>(INVALID_SOCKET);
  return 0;
}

int UdpSocket::send(const std::vector<std::uint8_t>& buffer)
{
  // send buffer via UDP socket
  return sendto(m_socket,
                reinterpret_cast<const char*>(buffer.data()),
                static_cast<int>(buffer.size()),
                0,
                (struct sockaddr*)&m_udpAddr,
                sizeof(m_udpAddr));
}

int UdpSocket::recv(std::vector<std::uint8_t>& buffer, std::size_t maxBytesToReceive)
{
  // receive from UDP Socket
  buffer.resize(maxBytesToReceive);
  char* pBuffer = reinterpret_cast<char*>(buffer.data());

  return ::recv(m_socket, pBuffer, static_cast<int>(maxBytesToReceive), 0);
}

int UdpSocket::read(std::vector<std::uint8_t>& buffer, std::size_t nBytesToReceive)
{
  // receive from TCP Socket
  buffer.resize(nBytesToReceive);
  char* pBuffer = reinterpret_cast<char*>(buffer.data());

  int bytesReceived = 0;
  while (nBytesToReceive > 0)
  {
    bytesReceived = ::recv(m_socket, pBuffer, static_cast<int>(nBytesToReceive), 0);

    if (bytesReceived == SOCKET_ERROR || bytesReceived == 0)
    {
      return false;
    }
    pBuffer += bytesReceived;
    nBytesToReceive -= bytesReceived;
  }
  pBuffer = NULL;
  return bytesReceived;
}

} // namespace visionary
