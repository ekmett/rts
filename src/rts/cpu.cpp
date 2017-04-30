#include "cpu.hpp"
#include "attribute.hpp"

#include <stdio.h>

#if defined(_WIN32) || defined(_WIN64)
#define PROP_WINDOWS
#include <intrin.h>
#endif

#ifndef __arm__
#ifndef PROP_WINDOWS
static RTS_ALWAYS_INLINE void __cpuid(int info[4], int infoType) noexcept {
  __asm__ __volatile__ (
      "cpuid"
    : "=a" (info[0]), "=b" (info[1]), "=c" (info[2]), "=d" (info[3])
    : "0" (infoType)
  );
}

static RTS_ALWAYS_INLINE void __cpuidex(int info[4], int level, int count) noexcept {
  __asm__ __volatile__ (
      "xchg{l}\t{%%}ebx, %1\n\t"
      "cpuid\n\t"
      "xchg{l}\t{%%}ebx, %1\n\t"
    : "=a" (info[0]), "=b" (info[1]), "=c" (info[2]), "=d" (info[3])
    : "0" (level), "2" (count)
  );
}
#endif

static RTS_ALWAYS_INLINE bool os_has_avx_support() noexcept {
#ifdef PROP_WINDOWS
  unsigned long long xcrFeatureMask = _xgetbv(_XCR_XFEATURE_ENABLED_MASK);
  return (xcrFeatureMask & 0x6) == 0x6;
#else
  int rEAX, rEDX;
  __asm__ __volatile__ (
      ".byte 0x0f, 0x01, 0xd0" // xgetbv
    : "=a" (rEAX), "=d" (rEDX)
    : "c" (0)
  );
  return (rEAX & 6) == 6;
#endif
}

static RTS_ALWAYS_INLINE bool os_has_avx512_support() noexcept {
#ifdef PROP_WINDOWS
  unsigned long long xcrFeatureMask = _xgetbv(_XCR_XFEATURE_ENABLED_MASK);
  return (xcrFeatureMask & 0x6) == 0x6;
#else
  int rEAX, rEDX;
  __asm__ __volatile__ (
      ".byte 0x0f, 0x01, 0xd0" // xgetbv
    : "=a" (rEAX), "=d" (rEDX)
    : "c" (0)
  );
  return (rEAX & 0xe6) == 0xe6;
#endif
}
#endif


namespace rts {
  isa system_isa() noexcept {
#ifdef __arm__
    return isa::arm_neon; // "ARM NEON"; // 8
#else
    int info[4];
    __cpuid(info,1);
    int info2[4];
    __cpuidex(info2,7,0);

    if ((info[2] & (1 << 27)) != 0 &&  // OSXSAVE
        (info2[1] & (1 <<  5)) != 0 && // AVX2
        (info2[1] & (1 << 16)) != 0 && // AVX512 F
      os_has_avx512_support()) {
      // We need to verify that AVX2 is also available,
      // as well as AVX512, because our targets are supposed
      // to use both.

      if ((info2[1] & (1 << 17)) != 0 && // AVX512 DQ
          (info2[1] & (1 << 28)) != 0 && // AVX512 CDI
          (info2[1] & (1 << 30)) != 0 && // AVX512 BW
          (info2[1] & (1 << 31)) != 0) { // AVX512 VL
        return isa::avx512_skx; // "SKX"; // 7
      } else if ((info2[1] & (1 << 26)) != 0 && // AVX512 PF
                 (info2[1] & (1 << 27)) != 0 && // AVX512 ER
                 (info2[1] & (1 << 28)) != 0) { // AVX512 CDI
        return isa::avx512_knl; // "KNL"; // 6
      }
      // If it's unknown AVX512 target, fall through and use AVX2
      // or whatever is available in the machine.
    }

    if ((info[2] & (1 << 27)) != 0 && // OSXSAVE
        (info[2] & (1 << 28)) != 0 &&
        os_has_avx_support()) {  // AVX
      // AVX1 for sure....
      // Ivy Bridge?
      if ((info[2] & (1 << 29)) != 0 &&  // F16C
          (info[2] & (1 << 30)) != 0) {  // RDRAND
        // So far, so good.  AVX2?
        if ((info2[1] & (1 << 5)) != 0 && // AVX2
            (info[2] & (1 << 12)) != 0) { // FMA3 -- EAK
          return isa::avx2; // "AVX2 (codename Haswell)"; // 5
        } else {
          return isa::avx11; // "AVX1.1 (codename Ivy Bridge)"; // 4
        }
      }
      // Regular AVX
      return isa::avx; // "AVX (codename Sandy Bridge)"; // 3
    } else if ((info[2] & (1 << 19)) != 0) {
      return isa::sse4; //  "SSE4"; // 2
    } else if ((info[3] & (1 << 26)) != 0) {
      return isa::sse2; // "SSE2"; // 1
    } else {
      return isa::error; // "Error"; // 0
    }
#endif
  }
} // namespace

/*
  Based on https://github.com/ispc/ispc/blob/master/check_isa.cpp

  Copyright (c) 2013-2015, Intel Corporation
  All rights reserved.
  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are
  met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of Intel Corporation nor the names of its
      contributors may be used to endorse or promote products derived from
      this software without specific prior written permission.
   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
   IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
   TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
   PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
   OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
   PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
   LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
   NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/


#ifdef TEST
int main() {
 printf("%d\n", rts::system_isa());
}
#endif
