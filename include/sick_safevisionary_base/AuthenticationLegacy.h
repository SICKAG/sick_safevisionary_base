//
// Copyright note: Redistribution and use in source, with or without modification, are permitted.
//
// Created: December 2019
//
// SICK AG, Waldkirch
// email: TechSupport0905@sick.de

#pragma once
#include "VisionaryControl.h"

namespace visionary {

class AuthenticationLegacy : public IAuthentication
{
public:
  explicit AuthenticationLegacy(VisionaryControl& vctrl);
  virtual ~AuthenticationLegacy();

  virtual bool login(UserLevel userLevel, const std::string& password);
  virtual bool logout();

private:
  VisionaryControl& m_VisionaryControl;
};

} // namespace visionary
