/// \copyright Redistribution and use in source, with or without modification, are permitted.
///
/// Definition of the interface used for authentication at a SICK SOPAS device
///
/// \email TechSupport0905@sick.de
///
/// \version 0.0.1

#pragma once
#include <string>

namespace visionary {

class IAuthentication
{
public:
  /// Available CoLa user levels.
  enum class UserLevel : int8_t
  {
    RUN               = 0,
    OPERATOR          = 1,
    MAINTENANCE       = 2,
    AUTHORIZED_CLIENT = 3,
    SERVICE           = 4
  };

  virtual ~IAuthentication(){};

  virtual bool login(UserLevel userLevel, const std::string& password) = 0;
  virtual bool logout()                                                = 0;
};

} // namespace visionary
