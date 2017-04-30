#pragma once

#if defined(__LP64__) || defined(_WIN64) || (defined(__x86_64__) && !defined(__ILP32__) ) || defined(_M_X64) || defined(__ia64) || defined (_M_IA64) || defined(__aarch64__) || defined(__powerpc64__)
#define RTS_64 1
#else
#define RTS_32 1
#endif

// cpu detection
namespace rts {
  /// instruction set architecture
  enum isa : int { 
    error = 0,
    sse2 = 1,
    sse4 = 2,
    avx = 3,
    avx11 = 4,
    avx2 = 5,
    avx512_knl = 6,
    avx512_skx = 7,
    max_intel = avx512_skx,
    amd_neon = 8,
    max_isa = amd_neon
  };
  extern isa system_isa() noexcept;
}
