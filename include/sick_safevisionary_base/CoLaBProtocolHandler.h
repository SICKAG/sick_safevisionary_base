//
// Copyright note: Redistribution and use in source, with or without modification, are permitted.
//
// Created: November 2019
//
// SICK AG, Waldkirch
// email: TechSupport0905@sick.de

#pragma once
#include "CoLaCommand.h"
#include "IProtocolHandler.h"
#include "ITransport.h"

namespace visionary {

class CoLaBProtocolHandler : public IProtocolHandler
{
public:
  CoLaBProtocolHandler(ITransport& rTransport);
  ~CoLaBProtocolHandler();

  bool openSession(uint8_t sessionTimeout /*secs*/);
  void closeSession();

  // send cola cmd and receive cola response
  CoLaCommand send(CoLaCommand cmd);

private:
  ITransport& m_rTransport;
  uint8_t calculateChecksum(const std::vector<uint8_t>& buffer);
};

} // namespace visionary
