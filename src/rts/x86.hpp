#pragma once

#include "platform.hpp"
#include "attribute.hpp"
#include <cstdint>

#ifdef _WIN32
#include <intrin.h>
#endif

#include <immintrin.h>

#if defined(__BMI__) && defined(__GNUC__)
  #if !defined(_tzcnt_u32)
    #define rts_tzcnt_u32 __tzcnt_u32
  #endif
  #if !defined(_tzcnt_u64)
    #define rts_tzcnt_u64 __tzcnt_u64
  #endif
#endif

#ifndef rts_tzcnt_u32
#define rts_tzcnt_u32 _tzcnt_u32
#endif

#ifndef rts_tzcnt_u64
#define rts_tzcnt_u64 _tzcnt_u64
#endif

#if defined(__LZCNT__)
  #if !defined(_lzcnt_u32)
    #define rts_lzcnt_u32 __lzcnt32
  #endif
  #if !defined(_lzcnt_u64)
    #define rts_lzcnt_u64 __lzcnt64
  #endif
#endif

#ifndef rts_lzcnt_u32
  #define rts_lzcnt_u32 _lzcnt32
#endif

#ifndef rts_lzcnt_u64
  #define rts_lzcnt_u64 _lzcnt64
#endif

#if defined(_WIN32)
  #define NOMINMAX
  #include <windows.h>
#endif

/// @file rts/x86.hpp
/// @brief x86 architecture-specific intrinsics

namespace rts {
  /// bit scan forward
  static RTS_ALWAYS_INLINE RTS_CONST int bsf(std::uint32_t v) noexcept {
    #if ((__GNUC__ >= 4) || ((__GNUC__ == 3) && (__GNUC_MINOR__ >= 4)))
      #if defined(__AVX2__)
        return rts_tzcnt_u32(v);
      #else
        return __builtin_ctz(v);
      #endif
    #elif defined(_MSC_VER)
      #if defined(__AVX2__)
        return rts_tzcnt_u32(v);
      #else
        unsigned long r = 0; _BitScanForward(&r,v); return (int)r;
      #endif
    #else
      int r = 0; asm ("bsf %1,%0" : "=r"(r) : "r"(v)); return r;
    #endif
  }

#if !defined(_MSC_VER) || defined(_WIN64)
  static RTS_ALWAYS_INLINE RTS_CONST int bsf(std::uint64_t v) noexcept {
    #if ((__GNUC__ >= 4) || ((__GNUC__ == 3) && (__GNUC_MINOR__ >= 4)))
      #if defined(__AVX2__)
        return (int)rts_tzcnt_u64(v);
      #else
        return __builtin_ctzll(v);
      #endif
    #elif defined(_MSC_VER)
      #if defined(__AVX2__)
        return (int)rts_tzcnt_u64(v);
      #else
        unsigned long r = 0; _BitScanForward64(&r,v); return (int)r;
      #endif
    #else
      int r = 0;
      asm ("bsf %1,%0" : "=r"(r) : "r"(v));
      return r;
    #endif
  }
#endif

  static RTS_ALWAYS_INLINE int bscf(std::uint32_t & v) noexcept {
    int r = bsf(v);
    v &= v-1;
    return r;
  }

#if !defined(_MSC_VER) || defined(_WIN64)
  static RTS_ALWAYS_INLINE int bscf(std::uint64_t & v) noexcept {
    int r = bsf(v);
    v &= v-1;
    return r;
  }
#endif

/// @defgroup rtm RTM
/// @brief Restricted Transactional Memory
///
/// @{

#ifdef __RTM__
/// @def RTS_RTM
/// @brief defined if we're compiling with RTM support enabled.
#define RTS_RTM
#endif

#ifdef RTS_RTM
  static const std::uint32_t rtm_started = ~0U;            ///< result of rtm_begin if it works
  static const std::uint32_t rtm_status_explicit = 1U << 0; ///< rtm_abort called explicitly
  static const std::uint32_t rtm_status_retry    = 1U << 1; ///< may succeed on retry, always clear if bit 0 set
  static const std::uint32_t rtm_status_conflict = 1U << 2; ///< set if another logical processor conflicted
  static const std::uint32_t rtm_status_capacity = 1U << 3; ///< an internal buffer overflowed
  static const std::uint32_t rtm_status_debug    = 1U << 4; ///< a debug breakport was hit
  static const std::uint32_t rtm_status_nested   = 1U << 5; ///< abort during nested transaction

  static RTS_ALWAYS_INLINE RTS_CONST constexpr std::uint32_t rtm_status_code(std::uint32_t x) noexcept { // retrieve the argument to xabort
    return (x >> 24) & 0xff;
  }

  /// Attempts to begin a RTM transaction. Returns rtm_started if successful.
  static RTS_ALWAYS_INLINE std::uint32_t rtm_begin() noexcept {
    int ret = rtm_started;
    asm volatile(".byte 0xc7,0xf8 ; .long 0" : "+a" (ret) :: "memory");
    return ret;
  }

  /// End an RTM transaction.
  static RTS_ALWAYS_INLINE void rtm_end() noexcept {
    asm volatile(".byte 0x0f,0x01,0xd5" ::: "memory");
  }

  /// Abort an RTM transaction. May send back an 8 bit status code from the future.
  static RTS_ALWAYS_INLINE void rtm_abort(const std::uint32_t status) noexcept {
    asm volatile(".byte 0xc6,0xf8,%P0" :: "i" (status) : "memory");
  }

  /// Is RTM active?
  static RTS_ALWAYS_INLINE bool rtm_test() noexcept {
    bool r;
    asm volatile(".byte 0x0f,0x01,0xd6 ; setnz %0" : "=r" (r) :: "memory");
    return r;
  }
#endif
/// @}
}
