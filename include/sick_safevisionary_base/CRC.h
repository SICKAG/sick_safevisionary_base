//
// Copyright note: Redistribution and use in source, with or without modification, are permitted.
//
// Created: October 2018
//
// SICK AG, Waldkirch
// email: TechSupport0905@sick.de

#pragma once

#include <cstdint>

namespace visionary
{

//! Default initial value for CRC-32
#define CRC_DEFAULT_INIT_VALUE32   ((uint32_t)0xFFFFFFFFu)

//! Compute the CRC-32 value of a data block based on a start value.
uint32_t CRC_calcCrc32Block( const void* const pvData, uint32_t u32Length, uint32_t u32InitVal );

//! Compute the CRC-32C value of a data block based on a start value.
uint32_t CRC_calcCrc32CBlock( const void* const pvData, uint32_t u32Length, uint32_t u32InitVal );

}
