#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include "rts/platform.hpp"
#include "rts.hpp"
#include "type.hpp"

#include <cmath> // isnan

using namespace rts;
namespace mm = rts::vec_intrinsics;

template <class T, class A, class A1, class VAL> void require_eq (const vec<T,A> & u, const vec<T,A1> & v, VAL val) {
  SECTION(val) {
    for (int i=0;i<A::width;++i) {
      if( std::isnan(u[i]) && std::isnan(v[i]) ) {
        REQUIRE( true );
      } else {
        REQUIRE( u[i] == v[i] );
      }
    }
  }
}

#define RTS_TEST0(fun,T) \
  SECTION(#fun " " #T) { \
    require_eq( mm::fun<T,Arch0>(), \
                mm::fun<T,Arch1>(), \
                "void"); \
  }
#define RTS_TEST1(fun,T,val0) \
  SECTION(#fun " " #T) { \
    require_eq( mm::fun(vec<T,Arch0>(val0)), \
                mm::fun(vec<T,Arch1>(val0)), \
                #val0); \
  }
#define RTS_TEST2(fun,T,val0,val1) \
  SECTION(#fun " " #T) { \
    require_eq( mm::fun(vec<T,Arch0>(val0), vec<T,Arch0>(val1)), \
                mm::fun(vec<T,Arch1>(val0), vec<T,Arch1>(val1)), \
                #val0 "," #val1 ); \
  }

#define RTS_TEST2T(fun,T,T1,val0,val1) \
  SECTION(#fun " " #T "," #T1) { \
    require_eq( mm::fun(vec<T,Arch0>(val0), vec<T1,Arch0>(val1)), \
                mm::fun(vec<T,Arch1>(val0), vec<T1,Arch1>(val1)), \
                #val0 "," #val1 ); \
  }


template <class Arch0,class Arch1>
void compare_arch () {
  using std::int32_t;

  SECTION("add"){
    RTS_TEST2(add, float, 1.23, 4.56)
    RTS_TEST2(add, int32_t, 23, 45)
  }
  SECTION("sub"){
    RTS_TEST2(sub, float, 1.23, 4.56)
    RTS_TEST2(sub, int32_t, 23, 45)
  }
  SECTION("mul"){
    RTS_TEST2(mul, float, 1.23, 4.56)
    RTS_TEST2(mul, int32_t, 23, 45)
  }
  SECTION("div"){
    RTS_TEST2(div, float, 1.23, 4.56)
  }

  std::int32_t s1 = 0b0101010101010101010101010101010;
  std::int32_t s2 = 0b0011001100110011001100110011001;
  float f1 = *(float*)&s1;
  float f2 = *(float*)&s2;

  SECTION("andnot"){
    RTS_TEST2(andnot, float, f1, f2)
    RTS_TEST2(andnot, int32_t, s1, s2)
    RTS_TEST2T(andnot, float, int32_t, f1, s2)
    RTS_TEST2T(andnot, int32_t, float, s1, f2)
  }
  SECTION("and_"){
    RTS_TEST2(and_, float, f1, f2)
    RTS_TEST2(and_, int32_t, s1, s2)
    RTS_TEST2T(and_, float, int32_t, f1, s2)
    RTS_TEST2T(and_, int32_t, float, s1, f2)
  }
  SECTION("xor_"){
    RTS_TEST2(xor_, float, f1, f2)
    RTS_TEST2(xor_, int32_t, s1, s2)
    RTS_TEST2T(xor_, float, int32_t, f1, s2)
    RTS_TEST2T(xor_, int32_t, float, s1, f2)
  }
  SECTION("or_"){
    RTS_TEST2(or_, float, f1, f2)
    RTS_TEST2(or_, int32_t, s1, s2)
    RTS_TEST2T(or_, float, int32_t, f1, s2)
    RTS_TEST2T(or_, int32_t, float, s1, f2)
  }

  SECTION("floor"){
    RTS_TEST1(floor, float, 2.0)
    RTS_TEST1(floor, float, 2.1)
    RTS_TEST1(floor, float, 2.9)
    RTS_TEST1(floor, float, -2.0)
    RTS_TEST1(floor, float, -2.1)
    RTS_TEST1(floor, float, -2.9)
  }
  SECTION("max"){
    RTS_TEST2(max, float, 2.0, 2.1)
    RTS_TEST2(max, float, 2.1, 2.0)
    RTS_TEST2(max, float, 2.0, 2.0)
  }
  SECTION("min"){
    RTS_TEST2(min, float, 2.0, 2.1)
    RTS_TEST2(min, float, 2.1, 2.0)
    RTS_TEST2(min, float, 2.0, 2.0)
  }
  SECTION("cmp"){
    RTS_TEST2(cmp<_CMP_LT_OS>, float, 2.0, 2.0)
    RTS_TEST2(cmp<_CMP_LT_OS>, float, 2.0, 2.1)
    RTS_TEST2(cmp<_CMP_LT_OS>, float, 2.1, 2.0)
    RTS_TEST2(cmp<_CMP_GT_OS>, float, 2.0, 2.0)
    RTS_TEST2(cmp<_CMP_GT_OS>, float, 2.0, 2.1)
    RTS_TEST2(cmp<_CMP_GT_OS>, float, 2.1, 2.0)
  }
  SECTION("cmpeq"){
    RTS_TEST2(cmpeq, int32_t, 2, 2)
    RTS_TEST2(cmpeq, int32_t, 2, 3)
    RTS_TEST2(cmpeq, int32_t, 3, 2)
  }
  SECTION("setzero"){
    RTS_TEST0(setzero, float)
  }
  SECTION("cast"){
    RTS_TEST1(cast<float>, int32_t, 0xFFFFF)
    RTS_TEST1(cast<int32_t>, float, 1e3)
  }
  SECTION("cvt"){
    RTS_TEST1(cvt<float>, int32_t, 0xFFFFF)
    RTS_TEST1(cvt<int32_t>, float, 1e3)
  }
}

TEST_CASE("Validate target generic against avx", "[vec_intrinsics]") {
  #ifdef __AVX__
    //compare_arch<target::generic<4>,target::avx_4>(); //TODO SIGABRTs
  #endif // __AVX__
  #ifdef __AVX2__
    compare_arch<target::generic<8>,target::avx2_8>();
  #endif // __AVX2__
}
