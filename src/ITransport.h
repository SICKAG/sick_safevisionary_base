//
// Copyright note: Redistribution and use in source, with or without modification, are permitted.
// 
// Created: November 2019
// 
// SICK AG, Waldkirch
// email: TechSupport0905@sick.de

#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>
#include <string>

namespace visionary 
{

class ITransport
{
public:
	virtual ~ITransport() {}; // destructor, use it to call destructor of the inherit classes

  /// Send data on socket to device
  ///
  /// \e All bytes are sent on the socket. It is regarded as error if this is not possible.
  ///
  /// \param[in] buffer buffer containing the bytes that shall be sent.
  ///
  /// \return OS error code.
  virtual int send(const std::vector<std::uint8_t> &buffer) = 0;

  /// Receive data on socket to device
  ///
  /// Receive at most \a maxBytesToReceive bytes.
  ///
  /// \param[in] buffer buffer containing the bytes that shall be sent.
  /// \param[in] maxBytesToReceive maximum number of bytes to receive.
  ///
  /// \return number of received bytes, negative values are OS error codes.
  virtual int recv(std::vector<std::uint8_t>& buffer, std::size_t maxBytesToReceive) = 0;
  
  /// Read a number of bytes
  ///
  /// Contrary to recv this method reads precisely \a nBytesToReceive bytes.
  ///
  /// \param[in] buffer buffer containing the bytes that shall be sent.
  /// \param[in] maxBytesToReceive maximum number of bytes to receive.
  ///
  /// \return number of received bytes, negative values are OS error codes.
  virtual int read(std::vector<std::uint8_t>& buffer, std::size_t nBytesToReceive) = 0;

};

}
