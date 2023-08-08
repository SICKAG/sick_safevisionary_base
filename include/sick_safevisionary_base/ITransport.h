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

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace visionary {

class ITransport
{
public:
  virtual ~ITransport(){}; // destructor, use it to call destructor of the inherit classes

  /// Send data on socket to device
  ///
  /// \e All bytes are sent on the socket. It is regarded as error if this is not possible.
  ///
  /// \param[in] buffer buffer containing the bytes that shall be sent.
  ///
  /// \return OS error code.
  virtual int send(const std::vector<std::uint8_t>& buffer) = 0;

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

} // namespace visionary
