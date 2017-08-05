#define CATCH_CONFIG_MAIN
#include "rts/platform.hpp"
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
      vec<double,A> d(0.0), e(1.0);
      auto r = d + e;
      const std::uint32_t m = A::width_mask;
      REQUIRE(movemask(r == e) == m);
      REQUIRE(movemask(std::sin(e) == rts::vec<double,A>(std::sin(1.0))) == m);
      REQUIRE(movemask(std::atan2(e,e) == rts::vec<double,A>(std::atan2(1.0,1.0))) == m);
  }
}

TEST_CASE("vec", "[vec]") {
  arch_test<target::generic<1>>();
#ifdef __AVX__
  arch_test<target::avx_4>();
  arch_test<target::avx_8>();
#endif
#ifdef __AVX2__
  arch_test<target::avx2_8>();
#endif

}
