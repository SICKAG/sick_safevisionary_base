//
// Copyright note: Redistribution and use in source, with or without modification, are permitted.
//
// Created: December 2019
//
// SICK AG, Waldkirch
// email: TechSupport0905@sick.de

#include "sick_safevisionary_base/AuthenticationLegacy.h"
#include "sick_safevisionary_base/CoLaParameterReader.h"
#include "sick_safevisionary_base/CoLaParameterWriter.h"

namespace visionary {

AuthenticationLegacy::AuthenticationLegacy(VisionaryControl& vctrl)
  : m_VisionaryControl(vctrl)
{
}

AuthenticationLegacy::~AuthenticationLegacy() {}

bool AuthenticationLegacy::login(UserLevel userLevel, const std::string& password)
{
  CoLaCommand loginCommand =
    CoLaParameterWriter(CoLaCommandType::METHOD_INVOCATION, "SetAccessMode")
      .parameterSInt(static_cast<int8_t>(userLevel))
      .parameterPasswordMD5(password)
      .build();
  CoLaCommand loginResponse = m_VisionaryControl.sendCommand(loginCommand);


  if (loginResponse.getError() == CoLaError::OK)
  {
    return CoLaParameterReader(loginResponse).readBool();
  }
  return false;
}

bool AuthenticationLegacy::logout()
{
  CoLaCommand runCommand  = CoLaParameterWriter(CoLaCommandType::METHOD_INVOCATION, "Run").build();
  CoLaCommand runResponse = m_VisionaryControl.sendCommand(runCommand);

  if (runResponse.getError() == CoLaError::OK)
  {
    return CoLaParameterReader(runResponse).readBool();
  }
  return false;
}

} // namespace visionary
