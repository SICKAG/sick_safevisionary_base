//
// Copyright note: Redistribution and use in source, with or without modification, are permitted.
// 
// Created: August 2017
// 
// SICK AG, Waldkirch
// email: TechSupport0905@sick.de

#include <cassert>
#include "sick_safevisionary_base/VisionaryControl.h"
#include "sick_safevisionary_base/VisionaryEndian.h"
#include "sick_safevisionary_base/CoLaBProtocolHandler.h"
#include "sick_safevisionary_base/CoLa2ProtocolHandler.h"
#include "sick_safevisionary_base/TcpSocket.h"
#include "sick_safevisionary_base/ControlSession.h"
#include "sick_safevisionary_base/AuthenticationLegacy.h"
#include "sick_safevisionary_base/CoLaParameterWriter.h"
#include "sick_safevisionary_base/CoLaParameterReader.h"

namespace visionary 
{

VisionaryControl::VisionaryControl()
{
}

VisionaryControl::~VisionaryControl()
{
}

bool VisionaryControl::open(ProtocolType type, const std::string& hostname, uint8_t sessionTimeout_s)
{
  m_pProtocolHandler = nullptr;
  m_pTransport = nullptr;

  std::unique_ptr<TcpSocket> pTransport(new TcpSocket());
  
  if (pTransport->connect(hostname, htons(type)) != 0)
  {
    return false;
  }

  std::unique_ptr<IProtocolHandler> pProtocolHandler;

  switch (type)
  {
  case COLA_B:
    pProtocolHandler = std::unique_ptr<IProtocolHandler>(new CoLaBProtocolHandler(*pTransport));
    break;
  case COLA_2:
    pProtocolHandler = std::unique_ptr<IProtocolHandler>(new CoLa2ProtocolHandler(*pTransport));
    break;
  default:
    assert(false /* unsupported protocol*/);
    return false;
  }

  if (!pProtocolHandler->openSession(sessionTimeout_s))
  {
    pTransport->shutdown();
    return false;
  }

  std::unique_ptr <ControlSession> pControlSession;
  pControlSession = std::unique_ptr<ControlSession>(new ControlSession(*pProtocolHandler));

  std::unique_ptr <IAuthentication> pAuthentication;
  pAuthentication = std::unique_ptr<IAuthentication>(new AuthenticationLegacy(*this));

  m_pTransport       = std::move(pTransport);
  m_pProtocolHandler = std::move(pProtocolHandler);
  m_pControlSession  = std::move(pControlSession);
  m_pAuthentication  = std::move(pAuthentication);

  return true;
}

void VisionaryControl::close()
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

bool VisionaryControl::login(IAuthentication::UserLevel userLevel, const std::string password)
{
  return m_pAuthentication->login(userLevel, password);
}

bool VisionaryControl::logout()
{
  return m_pAuthentication->logout();
}

std::string VisionaryControl::getDeviceIdent()
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

bool VisionaryControl::startAcquisition() 
{
  CoLaCommand command = CoLaParameterWriter(CoLaCommandType::METHOD_INVOCATION, "PLAYSTART").build();
  
  CoLaCommand response = m_pControlSession->send(command);

  return response.getError() == CoLaError::OK;
}

bool VisionaryControl::stepAcquisition() 
{
  CoLaCommand command = CoLaParameterWriter(CoLaCommandType::METHOD_INVOCATION, "PLAYNEXT").build();
  CoLaCommand response = m_pControlSession->send(command);

  return response.getError() == CoLaError::OK;
}

bool VisionaryControl::stopAcquisition() 
{
  CoLaCommand command = CoLaParameterWriter(CoLaCommandType::METHOD_INVOCATION, "PLAYSTOP").build();
  CoLaCommand response = m_pControlSession->send(command);

  return response.getError() == CoLaError::OK;
}

bool VisionaryControl::getDataStreamConfig() 
{
  CoLaCommand command = CoLaParameterWriter(CoLaCommandType::METHOD_INVOCATION, "GetBlobClientConfig").build();
  CoLaCommand response = m_pControlSession->send(command);

  return response.getError() == CoLaError::OK;
}

CoLaCommand VisionaryControl::sendCommand(CoLaCommand & command)
{
  return m_pControlSession->send(command);
}

}
