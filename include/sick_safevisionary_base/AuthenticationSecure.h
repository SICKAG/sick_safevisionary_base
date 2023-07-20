//
// Copyright note: Redistribution and use in source, with or without modification, are permitted.
//
// Created: January 2022
//
// SICK AG, Waldkirch
// email: TechSupport0905@sick.de

#pragma once
#include "SafeVisionaryControl.h"
#include <array>

namespace visionary {

struct ChallengeRequest
{
  std::array<std::uint8_t, 16> challenge;
  std::array<std::uint8_t, 16> salt;
};

typedef std::array<std::uint8_t, 32> PasswordHash;
typedef std::array<std::uint8_t, 32> ChallengeResponse;


class AuthenticationSecure : public IAuthentication
{
public:
  explicit AuthenticationSecure(SafeVisionaryControl& vctrl);
  virtual ~AuthenticationSecure();

  virtual bool login(UserLevel userLevel, const std::string& password);
  virtual bool logout();

private:
  SafeVisionaryControl& m_VisionaryControl;

  PasswordHash CreatePasswortHash(UserLevel userLevel,
                                  const std::string& password,
                                  const ChallengeRequest& challengeRequest);
  ChallengeResponse CreateChallengeResponse(UserLevel userLevel,
                                            const std::string& password,
                                            const ChallengeRequest& challengeRequest);
};

} // namespace visionary
