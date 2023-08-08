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
#include <cstring>

#define ENDIAN_LITTLE

// Configuration of CoLa command byte order according to the target device
//
// Define COLA_BYTE_ORDER_ENDIAN_LITTLE in case of the following devices:
//   * SICK SafeVisionary2
//
// Define COLA_BYTE_ORDER_ENDIAN_BIG in case of the following devices:
//   * SICK Visionary-S
//   * SICK Visionary-T
//   * SICK Visionary-T VGA
//   * SICK Visionary-t Mini
//
#define COLA_BYTE_ORDER_ENDIAN_LITTLE
//#define COLA_BYTE_ORDER_ENDIAN_BIG

namespace visionary {

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

template <class T>
T readUnaligned(const void* ptr)
{
  T r;
  memcpy(&r, ptr, sizeof(T));
  return r;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -


template <typename TAlias, typename T>
inline T byteswapAlias(T val);

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

inline uint8_t byteswap(uint8_t val)
{
  return val;
}
inline int8_t byteswap(int8_t val)
{
  return byteswapAlias<uint8_t>(val);
}
inline char byteswap(char val)
{
  return byteswapAlias<uint8_t>(val);
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

inline uint16_t byteswap(uint16_t val)
{
  return ((val << 8) & 0xFF00) | ((val >> 8) & 0x00FF);
}
inline int16_t byteswap(int16_t val)
{
  return byteswapAlias<uint16_t>(val);
}
inline wchar_t byteswap(wchar_t val)
{
  return byteswapAlias<uint16_t>(val);
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

inline uint32_t byteswap(uint32_t val)
{
  val = ((val << 8) & 0xFF00FF00) | ((val >> 8) & 0x00FF00FF);
  return ((val << 16) & 0xFFFF0000) | ((val >> 16) & 0x0000FFFF);
}
inline int32_t byteswap(int32_t val)
{
  return byteswapAlias<uint32_t>(val);
}
inline float byteswap(float val)
{
  union
  {
    float f32;
    uint32_t u32;
  } v;
  v.f32 = val;
  v.u32 = byteswap(v.u32);

  return v.f32;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

inline uint64_t byteswap(uint64_t val)
{
  val = ((val << 8) & 0xFF00FF00FF00FF00) | ((val >> 8) & 0x00FF00FF00FF00FF);
  val = ((val << 16) & 0xFFFF0000FFFF0000) | ((val >> 16) & 0x0000FFFF0000FFFF);
  return ((val << 32) & 0xFFFFFFFF00000000) | ((val >> 32) & 0x00000000FFFFFFFF);
}
inline int64_t byteswap(int64_t val)
{
  return byteswapAlias<uint64_t>(val);
}
inline double byteswap(double val)
{
  union
  {
    double f64;
    uint64_t u64;
  } v;
  v.f64 = val;
  v.u64 = byteswap(v.u64);

  return v.f64;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

template <typename TAlias, typename T>
inline T byteswapAlias(T val)
{
  return static_cast<T>(byteswap(static_cast<TAlias>(val)));
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

#if defined ENDIAN_LITTLE
template <typename T>
inline T nativeToLittleEndian(T x)
{
  return x;
}

template <typename T>
inline T littleEndianToNative(T x)
{
  return x;
}

template <typename T>
inline T nativeToBigEndian(T x)
{
  return byteswap(x);
}

template <typename T>
inline T bigEndianToNative(T x)
{
  return byteswap(x);
}
#elif defined ENDIAN_BIG
template <typename T>
inline T nativeToLittleEndian(T x)
{
  return byteswap(x);
}

template <typename T>
inline T littleEndianToNative(T x)
{
  return byteswap(x);
}

template <typename T>
inline T nativeToBigEndian(T x)
{
  return x;
}

template <typename T>
inline T bigEndianToNative(T x)
{
  return x;
}
#else
#error Endianess is not defined, please define either LITTLE_ENDIAN or BIG_ENDIAN depending on the platform.
#endif

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

template <typename T>
inline T readUnalignBigEndian(const void* ptr)
{
  return bigEndianToNative<T>(readUnaligned<T>(ptr));
}

template <typename T>
inline T readUnalignLittleEndian(const void* ptr)
{
  return littleEndianToNative<T>(readUnaligned<T>(ptr));
}

#if defined COLA_BYTE_ORDER_ENDIAN_LITTLE
template <typename T>
inline T nativeToColaByteOrder(T x)
{
  return nativeToLittleEndian<T>(x);
}
template <typename T>
inline T readUnalignColaByteOrder(const void* ptr)
{
  return littleEndianToNative<T>(readUnaligned<T>(ptr));
}
#elif defined COLA_BYTE_ORDER_ENDIAN_BIG
template <typename T>
inline T nativeToColaByteOrder(T x)
{
  return nativeToBigEndian<T>(x);
}
template <typename T>
inline T readUnalignColaByteOrder(const void* ptr)
{
  return bigEndianToNative<T>(readUnaligned<T>(ptr));
}
#else
#error Endianess for CoLa byte order is not defined, please define either COLA_BYTE_ORDER_ENDIAN_LITTLE or COLA_BYTE_ORDER_ENDIAN_BIG depending on the target device.
#endif

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

} // namespace visionary
