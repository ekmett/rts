#define CATCH_CONFIG_MAIN
#include "catch.hpp"
#include "rts.hpp"
#include "type.hpp"

using namespace rts;

template <class T, class A, class B> bool het_eq(const vec<T,A> & lhs, const vec<T,B> & rhs) {
  static_assert(A::width == B::width, "vec size mismatch");
  for (int i=0;i<A::width;++i) {
     if (lhs.get(i) != rhs.get(i)) {
       std::cout << "mismatch at position " << i << "," << lhs.get(i) << "!=" << rhs.get(i) << "\n";
       return false;
     }
  }
  return true;
}

template <class T, class A, typename ... Args> void pod_arch_test(Args ... args) {
  using G = target::generic<A::width>;

  SECTION(type<T>()) {
    CHECK(sizeof(rts::vec<T,A>) == sizeof(T)*A::width);
    CHECK(het_eq(vec<T,G>(args...),vec<T,A>(args...)));
  }
}

template <class A> void arch_test() {
  SECTION(type<A>()) {
    pod_arch_test<int,A>(0);
    pod_arch_test<float,A>(0.0f);
    pod_arch_test<int*,A>(nullptr);
  }
}

TEST_CASE("pod", "[pod]") {
  arch_test<target::generic<1>>();
#ifdef __AVX__
  arch_test<target::avx_4>();
#endif
#ifdef __AVX2__
  arch_test<target::avx2_8>();
#endif
}
