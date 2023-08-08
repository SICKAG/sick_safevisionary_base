// -- BEGIN LICENSE BLOCK ----------------------------------------------
/*!
*  Copyright (C) 2023, SICK AG, Waldkirch, Germany
*  Copyright (C) 2023, FZI Forschungszentrum Informatik, Karlsruhe, Germany
*
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*    http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.

*/
// -- END LICENSE BLOCK ------------------------------------------------

#include "sick_safevisionary_base/AuthenticationSecure.h"
#include "sick_safevisionary_base/CoLaParameterReader.h"
#include "sick_safevisionary_base/CoLaParameterWriter.h"
#include "sick_safevisionary_base/SHA256.h"

namespace visionary {

namespace {
enum class ChallengeResponseResult : std::uint8_t
{
  SUCCESS           = 0u,
  INVALID_CLIENT    = 1u,
  NOT_ACCEPTED      = 2u,
  UNKNOWN_CHALLENGE = 3u,
  PWD_NOT_CHANGABLE = 4u,
  TIMELOCK_ACTIVE   = 5u
};
}

AuthenticationSecure::AuthenticationSecure(SafeVisionaryControl& vctrl)
  : m_VisionaryControl(vctrl)
{
}

AuthenticationSecure::~AuthenticationSecure() {}

PasswordHash AuthenticationSecure::CreatePasswortHash(UserLevel userLevel,
                                                      const std::string& password,
                                                      const ChallengeRequest& challengeRequest)
{
  PasswordHash passwordHash{};
  std::string passwordPrefix{};

  switch (userLevel)
  {
    case UserLevel::RUN: {
      passwordPrefix = "Run";
      break;
    }
    case UserLevel::OPERATOR: {
      passwordPrefix = "Operator";
      break;
    }
    case UserLevel::MAINTENANCE: {
      passwordPrefix = "Maintenance";
      break;
    }
    case UserLevel::AUTHORIZED_CLIENT: {
      passwordPrefix = "AuthorizedClient";
      break;
    }
    case UserLevel::SERVICE: {
      passwordPrefix = "Service";
      break;
    }
    default: {
      // return empty hash code in case of error
      return passwordHash;
      break;
    }
  }
  std::string separator          = ":";
  std::string passwordWithPrefix = passwordPrefix + ":SICK Sensor:" + password;

  hash_state hashState{};
  sha256_init(&hashState);

  sha256_process(&hashState,
                 reinterpret_cast<const uint8_t*>(passwordWithPrefix.c_str()),
                 static_cast<std::uint32_t>(passwordWithPrefix.size()));
  sha256_process(&hashState,
                 reinterpret_cast<const uint8_t*>(separator.c_str()),
                 static_cast<std::uint32_t>(separator.size()));
  sha256_process(&hashState,
                 challengeRequest.salt.data(),
                 static_cast<std::uint32_t>(challengeRequest.salt.size()));
  sha256_done(&hashState, passwordHash.data());

  return passwordHash;
}

ChallengeResponse AuthenticationSecure::CreateChallengeResponse(
  UserLevel userLevel, const std::string& password, const ChallengeRequest& challengeRequest)
{
  ChallengeResponse challengeResponse{};
  PasswordHash passwordHash = CreatePasswortHash(userLevel, password, challengeRequest);

  hash_state hashState{};
  sha256_init(&hashState);
  sha256_process(&hashState, passwordHash.data(), static_cast<std::uint32_t>(passwordHash.size()));
  sha256_process(&hashState,
                 challengeRequest.challenge.data(),
                 static_cast<std::uint32_t>(challengeRequest.challenge.size()));
  sha256_done(&hashState, challengeResponse.data());

  return challengeResponse;
}

bool AuthenticationSecure::login(UserLevel userLevel, const std::string& password)
{
  bool isLoginSuccessful{false};

  // create command to get the challenge
  CoLaCommand getChallengeCommand =
    CoLaParameterWriter(CoLaCommandType::METHOD_INVOCATION, "GetChallenge")
      .parameterUSInt(static_cast<uint8_t>(userLevel))
      .build();

  // send command and get the response
  CoLaCommand getChallengeResponse = m_VisionaryControl.sendCommand(getChallengeCommand);

  // check whether there occurred an error with the CoLa communication
  if (getChallengeResponse.getError() == CoLaError::OK)
  {
    // read and check response of GetChallenge command
    CoLaParameterReader coLaParameterReader = CoLaParameterReader(getChallengeResponse);
    if (static_cast<ChallengeResponseResult>(coLaParameterReader.readUSInt()) ==
        ChallengeResponseResult::SUCCESS)
    {
      ChallengeRequest challengeRequest{};
      for (std::uint32_t byteCounter = 0u; byteCounter < sizeof(challengeRequest.challenge);
           byteCounter++)
      {
        challengeRequest.challenge[byteCounter] = coLaParameterReader.readUSInt();
      }
      for (std::uint32_t byteCounter = 0u; byteCounter < sizeof(challengeRequest.salt);
           byteCounter++)
      {
        challengeRequest.salt[byteCounter] = coLaParameterReader.readUSInt();
      }

      ChallengeResponse challengeResponse =
        CreateChallengeResponse(userLevel, password, challengeRequest);

      CoLaParameterWriter coLaParameterWriter =
        CoLaParameterWriter(CoLaCommandType::METHOD_INVOCATION, "SetUserLevel");

      // add challenge response value to set user level command
      for (std::uint32_t byteCounter = 0u; byteCounter < challengeResponse.size(); byteCounter++)
      {
        coLaParameterWriter.parameterUSInt(challengeResponse[byteCounter]);
      }

      // add user Level to command and build it
      CoLaCommand getUserLevelCommand =
        coLaParameterWriter.parameterUSInt(static_cast<uint8_t>(userLevel)).build();
      CoLaCommand getUserLevelResponse = m_VisionaryControl.sendCommand(getUserLevelCommand);
      if (getUserLevelResponse.getError() == CoLaError::OK)
      {
        coLaParameterReader = CoLaParameterReader(getUserLevelResponse);
        if (static_cast<ChallengeResponseResult>(coLaParameterReader.readUSInt()) ==
            ChallengeResponseResult::SUCCESS)
        {
          isLoginSuccessful = true;
        }
      }
    }
  }
  return isLoginSuccessful;
}

bool AuthenticationSecure::logout()
{
  CoLaCommand runCommand  = CoLaParameterWriter(CoLaCommandType::METHOD_INVOCATION, "Run").build();
  CoLaCommand runResponse = m_VisionaryControl.sendCommand(runCommand);

  if (runResponse.getError() == CoLaError::OK)
  {
    return CoLaParameterReader(runResponse).readUSInt();
  }
  return false;
}

} // namespace visionary
