//
// Copyright note: Redistribution and use in source, with or without modification, are permitted.
//
// Created: November 2019
//
// SICK AG, Waldkirch
// email: TechSupport0905@sick.de

#include "sick_safevisionary_base/TcpSocket.h"

namespace visionary {

int TcpSocket::connect(const std::string& hostname, uint16_t port)
{
  int iResult = 0;

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
  m_socket = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (m_socket == INVALID_SOCKET)
  {
    return (int)INVALID_SOCKET;
  }

  //-----------------------------------------------
  // Bind the socket to any address and the specified port.
  sockaddr_in recvAddr;
  recvAddr.sin_family      = AF_INET;
  recvAddr.sin_port        = port;
  recvAddr.sin_addr.s_addr = inet_addr(hostname.c_str());

  iResult = ::connect(m_socket, (sockaddr*)&recvAddr, sizeof(recvAddr));

  if (iResult != 0)
  {
    return iResult;
  }

  // Set the timeout for the socket to 5 seconds
  long timeoutSeconds = 5L;
#ifdef _WIN32
  // On Windows timeout is a DWORD in milliseconds
  // (https://docs.microsoft.com/en-us/windows/desktop/api/winsock/nf-winsock-setsockopt)
  long timeoutMs = timeoutSeconds * 1000L;
  iResult = setsockopt(m_socket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeoutMs, sizeof(DWORD));
#else
  struct timeval tv;
  tv.tv_sec  = timeoutSeconds; /* 5 seconds Timeout */
  tv.tv_usec = 0L;             // Not init'ing this can cause strange errors
  iResult = setsockopt(m_socket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(struct timeval));
#endif

  return iResult;
}

int TcpSocket::openServer(uint16_t port)
{
  int iResult = 0;
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
  // Create a server TCP socket to be able to connect to TCP clients
  m_socketServer = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (m_socketServer == INVALID_SOCKET)
  {
    return (int)INVALID_SOCKET;
  }

  //-----------------------------------------------
  // Bind the socket to any address and the specified port.
  sockaddr_in server;
  server.sin_family      = AF_INET;
  server.sin_port        = port;
  server.sin_addr.s_addr = INADDR_ANY;

  iResult = bind(m_socketServer, (sockaddr*)&server, sizeof(server));
  if (iResult == 0)
  {
    iResult = listen(m_socketServer, 1);
  }
  return iResult;
}


int TcpSocket::openTcp(uint16_t port)
{
  int iResult = -1;

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
  // Create a TCP socket to be able to connect to TCP device
  m_socketTcp = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (m_socketTcp == INVALID_SOCKET)
  {
    std::printf("TCP socket is not created\n");
    return (int)INVALID_SOCKET;
  }

  //-----------------------------------------------
  // Bind the socket to any address and the specified port.
  sockaddr_in saddr;
  saddr.sin_family      = AF_INET;
  saddr.sin_port        = port;
  saddr.sin_addr.s_addr = INADDR_ANY;

  iResult = bind(m_socketTcp, (sockaddr*)&saddr, sizeof(saddr));

  return iResult;
}

bool TcpSocket::WaitForConnection()
{
  bool res = false;
  sockaddr_in client;
  SOCKET clientSocket;
  int size = sizeof(sockaddr_in);

  clientSocket = accept(m_socketServer, (sockaddr*)&client, (socklen_t*)&size);

  if (clientSocket != INVALID_SOCKET)
  {
    m_socket = clientSocket;
    res      = true;
    printf("Connected to IP: %s, Port: %d\n", inet_ntoa(client.sin_addr), client.sin_port);
  }
  return res;
}

int TcpSocket::shutdown()
{
  // Close the socket when finished receiving datagrams
#ifdef _WIN32
  closesocket(m_socket);
  closesocket(m_socketServer);
  closesocket(m_socketTcp);
  WSACleanup();
#else
  close(m_socket);
  close(m_socketServer);
  close(m_socketTcp);
#endif
  m_socket       = INVALID_SOCKET;
  m_socketServer = INVALID_SOCKET;
  m_socketTcp    = INVALID_SOCKET;

  return 0;
}

int TcpSocket::send(const std::vector<std::uint8_t>& buffer)
{
  // send buffer via TCP socket
  return ::send(m_socket, (char*)buffer.data(), static_cast<int>(buffer.size()), 0);
}

int TcpSocket::recv(std::vector<std::uint8_t>& buffer, std::size_t maxBytesToReceive)
{
  // receive from TCP Socket
  buffer.resize(maxBytesToReceive);
  char* pBuffer = reinterpret_cast<char*>(buffer.data());

  return ::recv(m_socket, pBuffer, static_cast<int>(maxBytesToReceive), 0);
}

int TcpSocket::read(std::vector<std::uint8_t>& buffer, std::size_t nBytesToReceive)
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
      return -1;
    }
    pBuffer += bytesReceived;
    nBytesToReceive -= bytesReceived;
  }
  pBuffer = NULL;
  return buffer.size();
}

} // namespace visionary
