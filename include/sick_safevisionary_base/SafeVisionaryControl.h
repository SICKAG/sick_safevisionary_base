//
// Copyright note: Redistribution and use in source, with or without modification, are permitted.
//
// Created: August 2017
//
// SICK AG, Waldkirch
// email: TechSupport0905@sick.de

#pragma once

#include "CoLaCommand.h"
#include "ControlSession.h"
#include "IAuthentication.h"
#include "IProtocolHandler.h"
#include "TcpSocket.h"
#include <cstdint>
#include <memory>
#include <string>

namespace visionary {
class SafeVisionaryControl
{
public:
  /// The numbers used for the protocols are the port numbers.
  enum ProtocolType
  {
    INVALID_PROTOCOL = -1,
    COLA_2           = 2122
  };

  /// Default session timeout in seconds
  static const uint8_t kSessionTimeout_s = 5u;

  SafeVisionaryControl();
  ~SafeVisionaryControl();

  /// Opens a connection to a SafeVisionary sensor
  ///
  /// \param[in] hostname name or IP address of the Visionary sensor.
  /// \param[in] sessionTimeout_s timeout of session in seconds
  ///
  /// \retval true The connection to the sensor successfully was established.
  /// \retval false The connection attempt failed; the sensor is either
  ///               - switched off or has a different IP address or name
  ///               - not available using for PCs network settings (different subnet)
  bool open(const std::string& hostname, uint8_t sessionTimeout_s = kSessionTimeout_s);

  /// Close a connection
  ///
  /// Closes the control connection. It is allowed to call close of a connection
  /// that is not open. In this case this call is a no-op.
  void close();

  /// Login to the device.
  ///
  /// \param[in] userLevel The user level to login as.
  /// \param[in] password   Password for the selected user level.
  /// \return error code, 0 on success
  bool login(IAuthentication::UserLevel userLevel, const std::string password);

  /// <summary>Logout from the device.</summary>
  /// <returns>True if logout was successful, false otherwise.</returns>
  bool logout();

  /// <summary>
  /// Get device information by calling the "DeviceIdent" method on the device.
  /// </summary>
  /// <returns>True if successful, false otherwise.</returns>
  std::string getDeviceIdent();

  /// <summary>Send a <see cref="CoLaBCommand" /> to the device and waits for the result.</summary>
  /// <param name="command">Command to send</param>
  /// <returns>The response.</returns>
  CoLaCommand sendCommand(CoLaCommand& command);

private:
  std::string receiveCoLaResponse();
  CoLaCommand receiveCoLaCommand();

  std::unique_ptr<TcpSocket> m_pTransport;
  std::unique_ptr<IProtocolHandler> m_pProtocolHandler;
  std::unique_ptr<IAuthentication> m_pAuthentication;
  std::unique_ptr<ControlSession> m_pControlSession;
};

} // namespace visionary
