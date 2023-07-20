//
// Copyright note: Redistribution and use in source, with or without modification, are permitted.
//
// Created: November 2019
//
// SICK AG, Waldkirch
// email: TechSupport0905@sick.de

#pragma once

#include "CoLaCommand.h"
#include <cstdint>

namespace visionary {

class IProtocolHandler
{
public:
  virtual bool openSession(uint8_t sessionTimeout /*secs*/) = 0;
  virtual void closeSession()                               = 0;
  virtual CoLaCommand send(CoLaCommand cmd)                 = 0;
};

} // namespace visionary
