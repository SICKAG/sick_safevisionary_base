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
#include <cstdint>
#include <string>

#include "ITransport.h"

// Include socket
#ifdef _WIN32 // Windows specific
#  include <Ws2tcpip.h>
#  include <winsock2.h>
// to use with other compiler than Visual C++ need to set Linker flag -lws2_32
#  ifdef _MSC_VER
#    pragma comment(lib, "ws2_32.lib")
#  endif
#else // Linux specific
#  include <arpa/inet.h>
#  include <netinet/in.h>
#  include <sys/socket.h>
#  include <sys/types.h>
#  include <unistd.h>

typedef int SOCKET;
#  define INVALID_SOCKET ((SOCKET)(~0))
#  define SOCKET_ERROR (-1)
#endif

namespace visionary {

class UdpSocket : public ITransport
{
public:
  UdpSocket();

  int connect(const std::string& hostname, uint16_t port);
  int bindPort(std::uint16_t port);
  int shutdown();

  int send(const std::vector<std::uint8_t>& buffer) override;
  int recv(std::vector<std::uint8_t>& buffer, std::size_t maxBytesToReceive) override;
  int read(std::vector<std::uint8_t>& buffer, std::size_t nBytesToReceive) override;

private:
  SOCKET m_socket;
  struct sockaddr_in m_udpAddr;

  int initSocket();
};

} // namespace visionary
