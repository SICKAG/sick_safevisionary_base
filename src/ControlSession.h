/// \copyright Redistribution and use in source, with or without modification, are permitted.
///
/// Definition of the control session interface
///
/// \email TechSupport0905@sick.de
///
/// \version 0.0.1

#pragma once
#include <string>
#include "CoLaCommand.h"
#include "IProtocolHandler.h"

namespace visionary 
{

class ControlSession
{
public:
  ControlSession(IProtocolHandler& ProtocolHandler);
  virtual ~ControlSession();

  //void login(IAuthentication::UserLevel userLevel, const std::string& password);
  //void logout();

  CoLaCommand prepareRead(const std::string& varname);
  CoLaCommand prepareWrite(const std::string& varname);
  CoLaCommand prepareCall(const std::string& varname);

  CoLaCommand send(const CoLaCommand& cmd);

private:
  IProtocolHandler& m_ProtocolHandler;
};

}
