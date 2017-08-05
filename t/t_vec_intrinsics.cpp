#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include "rts/platform.hpp"
#include "rts.hpp"
#include "type.hpp"

#include <cmath> // isnan

using namespace rts;
namespace mm = rts::vec_intrinsics;

template<typename T,typename A>
static vec<T,A> range(T i0,T step){
    vec<T,A> r;
    T t(i0);
    for (int i=0;i<A::width;++i) {
       r[i] = t;
       t += step;
    }
    return r;
}

template <class T, class A, class A1, class VAL> void require_eq (const vec<T,A> & u, const vec<T,A1> & v, VAL val) {
  SECTION(val) {
    for (int i=0;i<A::width;++i) {
      if( std::isnan((long double)u[i]) && std::isnan((long double)v[i]) ) {
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

#define RTS_TEST2R(fun,T,r0,r1) \
  SECTION(#fun " " #T) { \
    require_eq( mm::fun(range<T,Arch0>r0, range<T,Arch0>r1 ), \
                mm::fun(range<T,Arch1>r0, range<T,Arch1>r1 ), \
               #r0 "," #r1); \
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

  SECTION("range"){
     using tg8 = target::generic<8>;
     require_eq(vec<int32_t,tg8>({1,2,3,4,5,6,7,8}),range<int32_t,tg8>(1,1),"range-int");
     require_eq(vec<float,tg8>({1,3,5,7,9,11,13,15}),range<float,tg8>(1,2),"range-float");
  }

  SECTION("add"){
    RTS_TEST2(add, float, 1.23, 4.56)
    RTS_TEST2R(add, float, (1.23,1), (4.56,-1.0))
    RTS_TEST2(add, int32_t, 23, 45)
    RTS_TEST2R(add, int32_t, (23,5), (45,10))
  }
  SECTION("sub"){
    RTS_TEST2(sub, float, 1.23, 4.56)
    RTS_TEST2R(sub, float, (1.23,1), (4.56,-1.0))
    RTS_TEST2(sub, int32_t, 23, 45)
    RTS_TEST2R(sub, int32_t, (23,5), (45,10))
  }
  SECTION("mul"){
    RTS_TEST2(mul, float, 1.23, 4.56)
    RTS_TEST2R(mul, float, (1.23,1), (4.56,-1.0))
    RTS_TEST2(mul, int32_t, 23, 45)
    RTS_TEST2R(mul, int32_t, (23,5), (45,10))
  }
  SECTION("div"){
    RTS_TEST2(div, float, 1.23, 4.56)
    RTS_TEST2R(div, float, (1.23,1), (4.56,-1.0))
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

TEST_CASE("Cross-check targets", "[vec_intrinsics]") {
  SECTION("generic,generic") {
    compare_arch<target::generic<8>,target::generic<8>>();
  }
  #ifdef __AVX__
  SECTION("generic,avx_4") {
    compare_arch<target::generic<4>,target::avx_4>();
  }
  SECTION("generic,avx_8") {
    compare_arch<target::generic<8>,target::avx_8>();
  }
  #endif // __AVX__
  #ifdef __AVX2__
  SECTION("generic,avx2_8") {
    compare_arch<target::generic<8>,target::avx2_8>();
  }
  #endif // __AVX2__
}
