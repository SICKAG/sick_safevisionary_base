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

namespace visionary {

namespace CoLaError {
/// Possible CoLa errors
enum Enum
{

  /// Network error (not sent with messages).
  NETWORK_ERROR = -1,

  /// No error
  OK = 0,

  /// Wrong userlevel, access to method not allowed.
  METHOD_IN_ACCESS_DENIED = 1,

  /// Trying to access a method with an unknown Sopas index.
  METHOD_IN_UNKNOWN_INDEX = 2,

  /// Trying to access a variable with an unknown Sopas index
  VARIABLE_UNKNOWN_INDEX = 3,

  /// Local condition violated, e.g. giving a value that exceeds the minimum or maximum allowed
  /// value for this variable
  LOCAL_CONDITION_FAILED = 4,

  /// Invalid data given for variable, this errorcode is deprecated (is not used anymore)
  INVALID_DATA = 5,

  /// An error with unknown reason occurred, this errorcode is deprecated.
  UNKNOWN_ERROR = 6,

  /// The communication buffer was too small for the amount of data that should be serialised
  BUFFER_OVERFLOW = 7,

  /// More data was expected, the allocated buffer could not be filled.
  BUFFER_UNDERFLOW = 8,

  /// The variable that shall be serialised has an unknown type. This can only happen when there are
  /// variables in the firmware of the device that do not exist in the released description of the
  /// device. This should never happen.
  ERROR_UNKNOWN_TYPE = 9,

  /// It is not allowed to write values to this variable. Probably the variable is defined as
  /// read-only
  VARIABLE_WRITE_ACCESS_DENIED = 10,

  /// When using names instead of indices, a command was issued that the nameserver does not
  /// understand
  UNKNOWN_CMD_FOR_NAMESERVER = 11,

  /// The CoLa protocol specification does not define the given command, command is unknown
  UNKNOWN_COLA_COMMAND = 12,

  /// It is not possible to issue more than one command at a time to an SRT device.
  METHOD_IN_SERVER_BUSY = 13,

  /// An array was accessed over its maximum length (the famous 0xE)
  FLEX_OUT_OF_BOUNDS = 14,

  /// The event you wanted to register for does not exist, the index is unknown.
  EVENT_REG_UNKNOWN_INDEX = 15,

  /// The value does not fit into the value field, it is too large.
  COLA_VALUE_UNDERFLOW = 16,

  /// Character is unknown, probably not alphanumeric (CoLaA only).
  COLA_A_INVALID_CHARACTER = 17,

  /// Only when using SRTOS in the firmware and distributed variables this error can occur. It is an
  /// indication that no operating system message could be created. This happens when trying to GET
  /// a variable
  OSAI_NO_MESSAGE = 18,

  /// This is the same as OsaiNoMessage with the difference that it is thrown when trying to PUT a
  /// variable.
  OSAI_NO_ANSWER_MESSAGE = 19,

  /// Internal error in the firmware, probably a pointer to a parameter was null.
  INTERNAL = 20,

  /// The Sopas Hubaddress is either too short or too long.
  HUB_ADDRESS_CORRUPTED = 21,

  /// The Sopas Hubaddress is invalid, it can not be decoded (Syntax).
  HUB_ADDRESS_DECODING = 22,

  /// Too many hubs in the address.
  HUB_ADDRESS_ADDRESS_EXCEEDED = 23,

  /// When parsing a HubAddress an expected blank was not found. The HubAddress is not valid.
  HUB_ADDRESS_BLANK_EXPECTED = 24,

  /// An asynchronous method call was made although the device was built with
  /// "AsyncMethodsSuppressed". This is an internal error that should never happen in a released
  /// device.
  ASYNC_METHODS_ARE_SUPPRESSED = 25,

  /// Device was built with "ComplexArraysSuppressed" because the compiler does not allow
  /// recursions. But now  a complex array was found. This is an internal error that should never
  /// happen in a released device.
  COMPLEX_ARRAYS_NOT_SUPPORTED = 32,

  /// CoLa2 session can not be created, no more sessions available.
  SESSION_NO_RESOURCES = 33,

  /// The CoLa2 session id is not valid, either it timed out or it never existed.
  SESSION_UNKNOWN_ID = 34,

  /// Requested connection (probably to a Hub Device) could not be established.
  CANNOT_CONNECT = 35,

  /// The given PortId (for routing a CoLa2 telegram) does not exist.
  INVALID_PORT = 36,

  /// A UDP Scan is already running.
  SCAN_ALREADY_ACTIVE = 37,

  /// There are no more timer objects available (for SOPAS Scan).
  OUT_OF_TIMERS = 38,

  /// It is currently not allowed to write to the device, it is in RUN mode.
  WRITE_MODE_NOT_ENABLED = 39,

  /// Internal error with SOPAS Scan.
  SET_PORT_FAILED = 40,

  /// IoLink error: function temporarily not available.
  IO_LINK_FUNC_TEMP_NOT_AVAILABLE = 256,

  /// Unknown error, internally thrown if SOPAS Scan received an unknown command.
  UNKNOWN = 32767
};
} // namespace CoLaError

} // namespace visionary
