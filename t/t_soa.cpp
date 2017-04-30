#define CATCH_CONFIG_MAIN
#include "catch.hpp"
#include "rts.hpp"
#include "type.hpp"

using namespace rts;

template <class T, class A, class B> bool het_eq(const vec<T,A> & lhs, const vec<T,B> & rhs) {
  static_assert(A::width == B::width, "vec size mismatch");
  for (int i=0;i<A::width;++i) {
     if (lhs.get(i) != rhs.get(i)) return false;
  }
  return true;
}


template <class A> void arch_test() {
  SECTION(type<A>()) {
    // soa<int,50,A> x;
  }
}

TEST_CASE("soa", "[soa]") {
  arch_test<target::generic<1>>();
#ifdef __AVX__
  arch_test<target::avx_4>();
#endif
#ifdef __AVX2__
  arch_test<target::avx2_8>();
#endif
}
