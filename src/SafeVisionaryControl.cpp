//
// Copyright note: Redistribution and use in source, with or without modification, are permitted.
//
// Created: January 2022
//
// SICK AG, Waldkirch
// email: TechSupport0905@sick.de

#include "sick_safevisionary_base/SafeVisionaryControl.h"
#include "sick_safevisionary_base/AuthenticationLegacy.h"
#include "sick_safevisionary_base/AuthenticationSecure.h"
#include "sick_safevisionary_base/CoLa2ProtocolHandler.h"
#include "sick_safevisionary_base/CoLaBProtocolHandler.h"
#include "sick_safevisionary_base/CoLaParameterReader.h"
#include "sick_safevisionary_base/CoLaParameterWriter.h"
#include "sick_safevisionary_base/ControlSession.h"
#include "sick_safevisionary_base/TcpSocket.h"
#include "sick_safevisionary_base/VisionaryEndian.h"
#include <cassert>

namespace visionary {
SafeVisionaryControl::SafeVisionaryControl() {}

SafeVisionaryControl::~SafeVisionaryControl() {}

bool SafeVisionaryControl::open(const std::string& hostname, uint8_t sessionTimeout_s)
{
  m_pProtocolHandler = nullptr;
  m_pTransport       = nullptr;

  std::unique_ptr<TcpSocket> pTransport(new TcpSocket());

  if (pTransport->connect(hostname, htons(COLA_2)) != 0)
  {
    return false;
  }

  std::unique_ptr<IProtocolHandler> pProtocolHandler;
  pProtocolHandler = std::unique_ptr<IProtocolHandler>(new CoLa2ProtocolHandler(*pTransport));

  if (!pProtocolHandler->openSession(sessionTimeout_s))
  {
    pTransport->shutdown();
    return false;
  }

  std::unique_ptr<ControlSession> pControlSession;
  pControlSession = std::unique_ptr<ControlSession>(new ControlSession(*pProtocolHandler));

  std::unique_ptr<IAuthentication> pAuthentication;
  pAuthentication = std::unique_ptr<IAuthentication>(new AuthenticationSecure(*this));

  m_pTransport       = std::move(pTransport);
  m_pProtocolHandler = std::move(pProtocolHandler);
  m_pControlSession  = std::move(pControlSession);
  m_pAuthentication  = std::move(pAuthentication);

  return true;
}

void SafeVisionaryControl::close()
{
  if (m_pAuthentication)
  {
    (void)m_pAuthentication->logout();
    m_pAuthentication = nullptr;
  }
  if (m_pProtocolHandler)
  {
    m_pProtocolHandler->closeSession();
    m_pProtocolHandler = nullptr;
  }
  if (m_pTransport)
  {
    m_pTransport->shutdown();
    m_pTransport = nullptr;
  }
  if (m_pControlSession)
  {
    m_pControlSession = nullptr;
  }
}

bool SafeVisionaryControl::login(IAuthentication::UserLevel userLevel, const std::string password)
{
  return m_pAuthentication->login(userLevel, password);
}

bool SafeVisionaryControl::logout()
{
  return m_pAuthentication->logout();
}

std::string SafeVisionaryControl::getDeviceIdent()
{
  CoLaCommand command = CoLaParameterWriter(CoLaCommandType::READ_VARIABLE, "DeviceIdent").build();

  CoLaCommand response = m_pControlSession->send(command);
  if (response.getError() == CoLaError::OK)
  {
    return CoLaParameterReader(response).readFlexString();
  }
  else
  {
    return "";
  }
}

CoLaCommand SafeVisionaryControl::sendCommand(CoLaCommand& command)
{
  return m_pControlSession->send(command);
}

} // namespace visionary
