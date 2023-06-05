/// \copyright Redistribution and use in source, with or without modification, are permitted.
///
/// Definition of the controlsession interface
///
/// \email TechSupport0905@sick.de
///
/// \version 0.0.1

#include "sick_safevisionary_base/ControlSession.h"
#include "sick_safevisionary_base/CoLaParameterWriter.h"

namespace visionary 
{

ControlSession::ControlSession(IProtocolHandler& ProtocolHandler)
  : m_ProtocolHandler(ProtocolHandler)
{
}

ControlSession::~ControlSession()
{
}

CoLaCommand ControlSession::prepareRead(const std::string& varname)
{
  CoLaCommand cmd = CoLaParameterWriter(CoLaCommandType::READ_VARIABLE, varname.c_str()).build();
  return cmd;
}

CoLaCommand ControlSession::prepareWrite(const std::string& varname)
{
  CoLaCommand cmd = CoLaParameterWriter(CoLaCommandType::WRITE_VARIABLE, varname.c_str()).build();
  return cmd;
}

CoLaCommand ControlSession::prepareCall(const std::string& varname)
{
  CoLaCommand cmd = CoLaParameterWriter(CoLaCommandType::METHOD_INVOCATION, varname.c_str()).build();
  return cmd;
}

CoLaCommand ControlSession::send(const CoLaCommand& cmd)
{
  // ToDo: send command via CoLaProtocolHandler?
  // ProcolHandler needs to add e.g. header and checksum
  // Afterwards send to socket and receive the response.
  // return the response.
  return m_ProtocolHandler.send(cmd);
}

}
