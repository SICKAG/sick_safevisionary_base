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

#pragma once

#include <cstdint>

namespace visionary {

//! Default initial value for CRC-32
#define CRC_DEFAULT_INIT_VALUE32 ((uint32_t)0xFFFFFFFFu)

//! Compute the CRC-32 value of a data block based on a start value.
uint32_t CRC_calcCrc32Block(const void* const pvData, uint32_t u32Length, uint32_t u32InitVal);

//! Compute the CRC-32C value of a data block based on a start value.
uint32_t CRC_calcCrc32CBlock(const void* const pvData, uint32_t u32Length, uint32_t u32InitVal);

} // namespace visionary
