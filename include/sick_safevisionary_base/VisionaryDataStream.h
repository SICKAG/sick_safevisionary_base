//
// Copyright note: Redistribution and use in source, with or without modification, are permitted.
//
// Created: November 2019
//
// SICK AG, Waldkirch
// email: TechSupport0905@sick.de

#pragma once

#include "TcpSocket.h"
#include "VisionaryData.h"
#include <memory>

namespace visionary {

class VisionaryDataStream
{
public:
  VisionaryDataStream(std::shared_ptr<VisionaryData> dataHandler);
  ~VisionaryDataStream();

  /// Opens a connection to a Visionary sensor
  ///
  /// \param[in] hostname name or IP address of the Visionary sensor.
  /// \param[in] port     control command port of the sensor, usually 2112 for CoLa-B or 2122 for
  /// CoLa-2.
  ///
  /// \retval true The connection to the sensor successfully was established.
  /// \retval false The connection attempt failed; the sensor is either
  ///               - switched off or has a different IP address or name
  ///               - not available using for PCs network settings (different subnet)
  ///               - the protocol type or the port did not match. Please check your sensor
  ///               documentation.
  bool open(const std::string& hostname, std::uint16_t port);

  /// Close a connection
  ///
  /// Closes the connection. It is allowed to call close of a connection
  /// that is not open. In this case this call is a no-op.
  void close();

  bool syncCoLa() const;

  //-----------------------------------------------
  // Receive a single blob from the connected device and store it in buffer.
  // Returns true when valid frame completely received.
  bool getNextFrame();

private:
  std::shared_ptr<VisionaryData> m_dataHandler;
  std::unique_ptr<TcpSocket> m_pTransport;

  // Parse the Segment-Binary-Data (Blob data without protocol version and packet type).
  // Returns true when parsing was successful.
  bool parseSegmentBinaryData(const std::vector<uint8_t>::iterator itBuf);
};

} // namespace visionary
