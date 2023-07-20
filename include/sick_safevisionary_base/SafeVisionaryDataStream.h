//
// Copyright note: Redistribution and use in source, with or without modification, are permitted.
//
// Created: November 2019
//
// SICK AG, Waldkirch
// email: TechSupport0905@sick.de

#pragma once

#include "TcpSocket.h"
#include "UdpSocket.h"
#include "VisionaryData.h"
#include <memory>
#include <vector>

namespace visionary {
/// Meta data contained in a UDP header
struct UdpProtocolData
{
  uint16_t blobNumber;     ///< BLOB number, incremented for each new Blob
  uint16_t fragmentNumber; ///< fragment number, incremented for each new fragment of the Blob
  uint16_t dataLength;     ///< length of the payload within the fragment
  bool isLastFragment;     ///< flag whether this was the last fragment of a Blob
};

enum class DataStreamError
{
  OK,
  DATA_RECEIVE_TIMEOUT,
  CONNECTION_CLOSED,
  PARSE_XML_ERROR,
  INVALID_VERSION_UDP_HEADER,
  INVALID_PACKET_TYPE_UDP_HEADER,
  INVALID_LENGTH_UDP_HEADER,
  INVALID_CRC_UDP_HEADER,
  INVALID_BLOB_HEADER,
  INVALID_VERSION_BLOB_HEADER,
  INVALID_PACKET_TYPE_BLOB_HEADER,
  INVALID_BLOB_ID,
  INVALID_BLOB_NUMBER,
  INVALID_UDP_FRAGMENT_NUMBER,
  DATA_SEGMENT_DEPTHMAP_ERROR,
  DATA_SEGMENT_DEVICESTATUS_ERROR,
  DATA_SEGMENT_ROI_ERROR,
  DATA_SEGMENT_LOCALIOS_ERROR,
  DATA_SEGMENT_FIELDINFORMATION_ERROR,
  DATA_SEGMENT_LOGICSIGNALS_ERROR,
  DATA_SEGMENT_IMU_ERROR
};

class SafeVisionaryDataStream
{
public:
  SafeVisionaryDataStream(std::shared_ptr<VisionaryData> dataHandler);
  ~SafeVisionaryDataStream();

  /// Connects to the sensor data stream using the given UDP port
  ///
  /// \param[in] port    UDP port to bind
  ///
  /// \retval true The connection to the sensor data stream has been successfully established.
  /// \retval false The connection attempt failed; the sensor is either
  ///               - switched off or streams on a different UPD port
  ///               - not available using for PCs network settings (different subnet)
  ///               - the protocol type or the port did not match. Please check your sensor
  ///               documentation.
  bool openUdpConnection(std::uint16_t port);

  /// Connects to the sensor data stream using the given TCP port and given IPAddress
  bool openTcpConnection(std::uint16_t port, std::string deviceIpAddress);

  /// Closes the connection. It is allowed to call close of a connection
  /// that is not open. In this case this call is a no-op.
  void closeUdpConnection();

  // Close the Tcp connection
  void closeTcpConnection();

  /// Receive a single blob from the connected device and store it in buffer.
  ///
  /// \return Returns true when a complete blob has been successfully received.
  bool getNextBlobUdp();

  /// \return Returns true when a complete blob has been successfully received.
  bool getNextBlobTcp(std::vector<std::uint8_t>& receiveBufferPacketSize);

  /// Gets the last error which occurred while parsing the data stream.
  ///
  /// \return Returns the last error, OK in case there occurred no error
  DataStreamError getLastError();

  // start bytes of Blob data have been found
  bool getBlobStartTcp(std::vector<std::uint8_t>& receiveBufferPacketSize);

private:
  /// Shared pointer to the Visionary data handler
  std::shared_ptr<VisionaryData> m_dataHandler;

  /// Unique pointer the UDP socket used to receive the measurement data output stream
  std::unique_ptr<UdpSocket> m_pTransportUdp;

  /// Unique TCP socket used to receive the measurement data output stream
  TcpSocket m_pTransportTcp;

  /// Buffer which stores the received Blob data
  std::vector<uint8_t> m_blobDataBuffer;

  /// Number of current Blob
  uint16_t m_blobNumber;

  /// number of Blob data segments
  uint16_t m_numSegments;

  /// Offset in byte for each data segment
  std::vector<uint32_t> m_offsetSegment;

  /// Change counter for each data segment
  std::vector<uint32_t> m_changeCounter;

  /// Stores the last error which occurred while parsing the data stream
  DataStreamError m_lastDataStreamError;

  /// Gets the next fragment of the Blob data via the opened UDP socket.
  ///
  /// \param[out] receiveBuffer Vector which contains the received fragment
  /// \return Returns true in case the next fragment has been received successfully
  bool getNextFragment(std::vector<std::uint8_t>& receiveBuffer);

  /// Gets the next packet of the Blob data via the opened TCP socket.
  ///
  /// \param[out] receiveBuffer Vector which contains the received Packet
  /// \return the received size of the tcp packet
  int32_t getNextTcpReception(std::vector<std::uint8_t>& receiveBuffer);

  /// Parses and checks the UDP header of one UDP fragment.
  /// Furthermore it returns some metadata regarding the fragment.
  ///
  /// \param[in] buffer Vector which contains the received UDP telegram
  /// \param[out] udpProtocolData reference to the UDP protocol data which is gotten from the UDP
  /// telegram for later use \return Returns true in case the UDP header is valid
  bool parseUdpHeader(std::vector<std::uint8_t>& buffer, UdpProtocolData& udpProtocolData);

  /// Finds the start of the next Blob data.
  ///
  /// This function gets the next Blob data telegram until a telegram with the fragment number 0 is
  /// received. In case the Blob data start has been found, this telegram is stored in the internal
  /// BlobData buffer. \param[out] lastFragment true in case the Blob data consists only of one
  /// fragment
  bool getBlobStartUdp(bool& lastFragment);

  /// Parses and checks the Blob header of a complete Blob data telegram.
  /// In case the Blob header is valid, the offset and change counter of each Blob data segment is
  /// stored.
  ///
  /// \return Returns true in case the Blob header is valid
  bool parseBlobHeaderUdp();

  /// Parses and checks the Blob header of a complete Blob data telegram.
  /// In case the Blob header is valid
  ///
  /// \return Returns true in case the Blob header is valid
  bool parseBlobHeaderTcp();

  /// Parses the segments of the Blob data
  /// \return Returns true in case the parsing of the Blob data has been successful, otherwise
  /// returns false
  bool parseBlobData();
};

} // namespace visionary
