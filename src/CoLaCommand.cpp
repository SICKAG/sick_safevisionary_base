//
// Copyright note: Redistribution and use in source, with or without modification, are permitted.
// 
// Created: October 2018
// 
// SICK AG, Waldkirch
// email: TechSupport0905@sick.de

#include "CoLaCommand.h"

#include <string>
#include "VisionaryEndian.h"

namespace visionary 
{

CoLaCommand::CoLaCommand(CoLaCommandType::Enum commandType, CoLaError::Enum error, const char* name)
  : m_buffer()
  , m_type(commandType)
  , m_name(name)
  , m_parameterOffset(0)
  , m_error(error)
{

}

CoLaCommand::CoLaCommand(std::vector<uint8_t> buffer)
  : m_buffer(buffer)
  , m_type(CoLaCommandType::UNKNOWN)
  , m_name("")
  , m_parameterOffset(0)
  , m_error(CoLaError::OK)
{
  // Read type from header
  if (buffer.size() < 3)
    return;
  std::string typeStr(reinterpret_cast<char*>(&buffer[0]), 3);
  if (typeStr.compare("sRN") == 0) m_type = CoLaCommandType::READ_VARIABLE;
  else if (typeStr.compare("sRA") == 0) m_type = CoLaCommandType::READ_VARIABLE_RESPONSE;
  else if (typeStr.compare("sWN") == 0) m_type = CoLaCommandType::WRITE_VARIABLE;
  else if (typeStr.compare("sWA") == 0) m_type = CoLaCommandType::WRITE_VARIABLE_RESPONSE;
  else if (typeStr.compare("sMN") == 0) m_type = CoLaCommandType::METHOD_INVOCATION;
  else if (typeStr.compare("sAN") == 0) m_type = CoLaCommandType::METHOD_RETURN_VALUE;
  else if (typeStr.compare("sFA") == 0) m_type = CoLaCommandType::COLA_ERROR;

  if(m_type == CoLaCommandType::COLA_ERROR)
  {
    m_parameterOffset = 3; // sFA

    // Read error code
    m_error = static_cast<CoLaError::Enum>(readUnalignColaByteOrder<std::uint16_t>(&buffer[m_parameterOffset]));
  }
  else if (m_type == CoLaCommandType::NETWORK_ERROR)
  {
    m_error = CoLaError::NETWORK_ERROR;
  }
  else if (m_type != CoLaCommandType::UNKNOWN)
  {
    // Find name and parameter start
    for (size_t i = 4; i < buffer.size(); i++)
    {
      if (buffer.at(i) == ' ')
      {
        m_name = std::string(reinterpret_cast<const char*>(&buffer[4]), i - 4);
        m_parameterOffset = i + 1; // Skip space
        break;
      }
    }
  }
}

CoLaCommand::~CoLaCommand()
{
}

const std::vector<uint8_t>& CoLaCommand::getBuffer()
{
  return m_buffer;
}

CoLaCommandType::Enum CoLaCommand::getType()
{
  return m_type;
}

const char* CoLaCommand::getName()
{
  return m_name.c_str();
}

size_t CoLaCommand::getParameterOffset()
{
  return m_parameterOffset;
}

CoLaError::Enum CoLaCommand::getError()
{
  return m_error;
}

CoLaCommand CoLaCommand::networkErrorCommand()
{
  return CoLaCommand(CoLaCommandType::NETWORK_ERROR, CoLaError::NETWORK_ERROR, "");
}

}
