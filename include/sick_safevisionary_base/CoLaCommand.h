//
// Copyright note: Redistribution and use in source, with or without modification, are permitted.
// 
// Created: October 2018
// 
// SICK AG, Waldkirch
// email: TechSupport0905@sick.de

#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include "CoLaError.h"
#include "CoLaCommandType.h"

namespace visionary 
{

class CoLaCommand
{
private:
  std::vector<uint8_t> m_buffer;
  CoLaCommandType::Enum m_type;
  std::string m_name;
  size_t m_parameterOffset;
  CoLaError::Enum m_error;

  /// <summary>Construct a new <see cref="CoLaCommand" /> with the given command type, error, and name, but without any data.</summary>
  CoLaCommand(CoLaCommandType::Enum commandType, CoLaError::Enum error, const char* name);

public:
  /// <summary>Construct a new <see cref="CoLaCommand" /> from the given data buffer.</summary>
  CoLaCommand(std::vector<uint8_t> buffer);
  ~CoLaCommand();

  /// <summary>Get the binary data buffer.</summary>
  const std::vector<uint8_t>& getBuffer();

  /// <summary>Get the type of command.</summary>
  CoLaCommandType::Enum getType();

  /// <summary>Get the name of command.</summary>
  const char* getName();

  /// <summary>Get offset in bytes to where first parameter starts.</summary>
  size_t getParameterOffset();

  /// <summary>Get error.</summary>
  CoLaError::Enum getError();

  /// <summary>Create a command for network errors.</summary>
  static CoLaCommand networkErrorCommand();
};

}
