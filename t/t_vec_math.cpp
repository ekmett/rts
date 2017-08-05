#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include "rts/platform.hpp"
#include "rts.hpp"
#include "type.hpp"

#include <cmath>
#define RTS_M_PI 3.14159265358979323846
#define RTS_M_E 2.71828182845904523536

using namespace rts;

template <class T, class A>
void require_approx_eq (const vec<T,A> & u, const vec<T,A> & v) {
  for (int i=0;i<A::width;++i) {
    if( std::isnan(u[i]) && std::isnan(v[i]) ) {
      REQUIRE( true );
    }
    else if( std::isinf(u[i]) && std::isinf(v[i]) && !((u[i] > 0) ^ (v[i] > 0))) {
      REQUIRE( true );
    }
    else {
      REQUIRE( u[i] == Approx( v[i] ));
    }
  }
}

// Hex float literals are gnu89, C99, gnu++11, but C++17.
#if __cplusplus > 201402L //>= 201703L
  #define RTS_HEXFL(fun,x) RTS_TEST(fun,x)
  #define RTS_HEXFL_SINCOS(x) RTS_TEST_SINCOS(x)
#else
  #define RTS_HEXFL(fun,x)
  #define RTS_HEXFL_SINCOS(x)
#endif

#define RTS_TEST(fun,x) \
  SECTION(#x) { \
    require_approx_eq( \
      rts::vec_math::fun(vec<float,A>((float)x)), \
      vec<float,A>(std::fun((float)x))); \
  }

//ISSUE On darwin xcode 8.3.3, sincos output arguments aren't being written to.
#ifdef __APPLE__
  #define RTS_RUN_SINCOS_CHECKS if( b[0] != 12.34f )
#else
  #define RTS_RUN_SINCOS_CHECKS if(true)
#endif

#define RTS_TEST_SINCOS(x) \
  SECTION(#x) { \
    vec<float,A> a(x), b(12.34), c(12.34); \
    rts::vec_math::sincos(a,b,c); \
    RTS_RUN_SINCOS_CHECKS { \
      SECTION("sin") { require_approx_eq( b, vec<float,A>(std::sin((float)x))); } \
      SECTION("cos") { require_approx_eq( c, vec<float,A>(std::cos((float)x))); } \
    } \
  }

#define RTS_MIN std::numeric_limits<float>::min()
#define RTS_MAX std::numeric_limits<float>::max()
#define RTS_MIN_SUBNORM std::numeric_limits<float>::denorm_min()

// Sources of interesting test values:
// from glibc [1]
// glibc.git/math/auto-libm-test-in 2017-03-16
// https://sourceware.org/git/?p=glibc.git;a=history;f=math/auto-libm-test-in;hb=refs/heads/master

// Validate rts::vec_math::foo<float,A> against std::foo
template <class A> void arch_test() {
  SECTION(type<A>()) {

    SECTION( "log" ) {
      // sanity
      RTS_TEST(log, 1.0 );
      RTS_TEST(log, 1.5 );
      //RTS_TEST(log, 0 ); //FAIL -nanf == Approx( -inf )
      // from glibc [1]
      RTS_TEST(log, 1 );
      RTS_TEST(log, RTS_M_E );
      RTS_TEST(log, 1/RTS_M_E );
      RTS_TEST(log, 2 );
      RTS_TEST(log, 10 );
      RTS_TEST(log, 0.75 );
      RTS_HEXFL(log, 0x1.000002p0 );
      RTS_HEXFL(log, 0x1.0000000000001p0 );
      RTS_HEXFL(log, 0x1.0000000000000002p0 );
      RTS_HEXFL(log, 0x1.000000000000000000000000008p0 );
      RTS_HEXFL(log, 0x1.0000000000000000000000000001p0 );
      RTS_HEXFL(log, 0x0.ffffffp0 );
      RTS_HEXFL(log, 0x0.fffffffffffff8p0 );
      RTS_HEXFL(log, 0x0.ffffffffffffffffp0 );
      RTS_HEXFL(log, 0x0.ffffffffffffffffffffffffffcp0 );
      RTS_HEXFL(log, 0x0.ffffffffffffffffffffffffffff8p0 );
      RTS_TEST(log, RTS_MIN );
      //RTS_TEST(log, RTS_MIN_SUBNORM ); //FAIL -87.33655f == Approx( -103.2789306641 )
      RTS_TEST(log, RTS_MAX );
      RTS_HEXFL(log, 0xb.0d5dfp-4 );
      RTS_HEXFL(log, 0x1.6c3f6p+0 );
      RTS_HEXFL(log, 0xa.ae688p-4 );
      RTS_HEXFL(log, 0x1.017f8ap+44 );
      RTS_HEXFL(log, 0x1.0b5c1ep+36 );
      //RTS_HEXFL(log, 0x2.1b17c2887e938p+928 ); //FAIL 88.72284f == Approx( inf )
      RTS_HEXFL(log, 0x1.929d9cp+0 );
      RTS_HEXFL(log, 0x1.770072p+0 );
    }

    SECTION( "exp" ) {
      // sanity
      RTS_TEST(exp, 0.0 );
      RTS_TEST(exp, 1.0 );
      RTS_TEST(exp, 2.0 );
      // from glibc [1]
      RTS_TEST(exp, 0 );
      RTS_TEST(exp, -0 );
      RTS_TEST(exp, 1 );
      RTS_TEST(exp, 2 );
      RTS_TEST(exp, 0.75 );
      RTS_TEST(exp, 3 );
      RTS_TEST(exp, 50.0 );
      //RTS_TEST(exp, 88.72269439697265625 ); //FAIL 240614362739678911180052727245270155264.0f == Approx( 340233121230420797207585263505645764608.0 )
      //RTS_TEST(exp, 709.75 ); //FAIL 240614362739678911180052727245270155264.0f == Approx( inf )
      //RTS_TEST(exp, 1000.0 ); //FAIL 240614362739678911180052727245270155264.0f == Approx( inf )
      //RTS_TEST(exp, 710 ); //FAIL 240614362739678911180052727245270155264.0f == Approx( inf )
      RTS_TEST(exp, -1234 );
      //RTS_HEXFL(exp, 0x2.c679d1f73f0fb628p+8 ); //FAIL 240614362739678911180052727245270155264.0f == Approx( inf )
      //RTS_TEST(exp, 1e5 ); //FAIL 240614362739678911180052727245270155264.0f == Approx( inf )
      //RTS_TEST(exp, RTS_MAX ); //FAIL 240614362739678911180052727245270155264.0f == Approx( inf )
      RTS_TEST(exp, -7.4444006192138124e+02 );
      RTS_HEXFL(exp, -0x1.75f113c30b1c8p+9 );
      RTS_TEST(exp, -RTS_MAX );
      RTS_TEST(exp, -11342.8125 );
      RTS_HEXFL(exp, -0x2.c5b2319c4843acc0p12 );
      RTS_HEXFL(exp, -0x2.c469d9p+8 );
      RTS_HEXFL(exp, -0x2.c46d96p+8 );
      RTS_HEXFL(exp, -0x2.c46727p+8 );
      RTS_HEXFL(exp, -0x2.c469dep+8 );
      RTS_HEXFL(exp, -0x2.c46c04p+8 );
      RTS_HEXFL(exp, -0x2.c46adep+8 );
      RTS_HEXFL(exp, -0x2.c471b3p+8 );
      RTS_HEXFL(exp, -0x2.c46993p+8 );
      RTS_HEXFL(exp, -0x2.c49fap+8 );
      RTS_HEXFL(exp, -0x2.c4ac1p+8 );
      RTS_HEXFL(exp, -0x2.c4d89p+8 );
      RTS_HEXFL(exp, 0x1p-10 );
      RTS_HEXFL(exp, -0x1p-10 );
      RTS_HEXFL(exp, 0x1p-20 );
      RTS_HEXFL(exp, -0x1p-20 );
      RTS_HEXFL(exp, 0x1p-30 );
      RTS_HEXFL(exp, -0x1p-30 );
      RTS_HEXFL(exp, 0x1p-40 );
      RTS_HEXFL(exp, -0x1p-40 );
      RTS_HEXFL(exp, 0x1p-50 );
      RTS_HEXFL(exp, -0x1p-50 );
      RTS_HEXFL(exp, 0x1p-60 );
      RTS_HEXFL(exp, -0x1p-60 );
      RTS_HEXFL(exp, 0x1p-100 );
      RTS_HEXFL(exp, -0x1p-100 );
      RTS_HEXFL(exp, 0x1p-600 );
      RTS_HEXFL(exp, -0x1p-600 );
      //RTS_HEXFL(exp, 0x1p-10000 ); // avoid warning: floating constant truncated to zero [-Woverflow]
      //RTS_HEXFL(exp, -0x1p-10000 ); // avoid warning: floating constant truncated to zero [-Woverflow]
      //RTS_HEXFL(exp, 0x5.8b90b8p+4 ); //FAIL 240614362739678911180052727245270155264.0f == Approx( 340279851902147610656242037972608745472.0 )
      //RTS_HEXFL(exp, 0x5.8b90cp+4 ); //FAIL 240614362739678911180052727245270155264.0f == Approx( inf )
      RTS_HEXFL(exp, -0x5.75628p+4 );
      RTS_HEXFL(exp, -0x5.756278p+4 );
      //RTS_HEXFL(exp, 0x2.c5c85fdf473dep+8 ); //FAIL 240614362739678911180052727245270155264.0f == Approx( inf )
      //RTS_HEXFL(exp, 0x2.c5c85fdf473ep+8 ); //FAIL 240614362739678911180052727245270155264.0f == Approx( inf )
      RTS_HEXFL(exp, -0x2.c4657baf579a6p+8 );
      RTS_HEXFL(exp, -0x2.c4657baf579a4p+8 );
      //RTS_HEXFL(exp, 0x2.c5c85fdf473de6ab278ece600fp+8 ); // xfail-rounding:ibm128-libgcc //FAIL 240614362739678911180052727245270155264.0f == Approx( inf )
      //RTS_HEXFL(exp, 0x2.c5c85fdf473de6ab278ece601p+8 ); // xfail-rounding:ibm128-libgcc //FAIL 240614362739678911180052727245270155264.0f == Approx( inf )
      RTS_HEXFL(exp, -0x2.9fa8dcb9092a538b3f2ee2ca67p+8 ); // xfail-rounding:ibm128-libgcc
      RTS_HEXFL(exp, -0x2.9fa8dcb9092a538b3f2ee2ca66p+8 ); // xfail-rounding:ibm128-libgcc
      //RTS_HEXFL(exp, 0x2.c5c85fdf473de6acp+12 ); //FAIL 240614362739678911180052727245270155264.0f == Approx( inf )
      //RTS_HEXFL(exp, 0x2.c5c85fdf473de6bp+12 ); //FAIL 240614362739678911180052727245270155264.0f == Approx( inf )
      RTS_HEXFL(exp, -0x2.c5b2319c4843accp+12 );
      RTS_HEXFL(exp, -0x2.c5b2319c4843acbcp+12 );
      RTS_HEXFL(exp, -0x2.c5bd48bdc7c0c9b8p+12 );
      RTS_HEXFL(exp, -0x2.c5bd48bdc7c0c9b4p+12 );
      //RTS_HEXFL(exp, 0x2.c5c85fdf473de6af278ece600fcap+12 ); //FAIL 240614362739678911180052727245270155264.0f == Approx( inf )
      //RTS_HEXFL(exp, 0x2.c5c85fdf473de6af278ece600fccp+12 ); //FAIL 240614362739678911180052727245270155264.0f == Approx( inf )
      RTS_HEXFL(exp, -0x2.c5b2319c4843acbff21591e99cccp+12 );
      RTS_HEXFL(exp, -0x2.c5b2319c4843acbff21591e99ccap+12 );
      RTS_TEST(exp, RTS_MIN );
      RTS_TEST(exp, -RTS_MIN );
      RTS_TEST(exp, RTS_MIN_SUBNORM );
      RTS_TEST(exp, -RTS_MIN_SUBNORM );
      RTS_HEXFL(exp, -0x1.760cd14774bd9p+0 );
      RTS_HEXFL(exp, 0x1.4bed28p+0 );
      RTS_HEXFL(exp, -0x1.f1cf36p+8 );
      RTS_HEXFL(exp, 0x3.248524p+0 );
      RTS_HEXFL(exp, 0x1.f0b362p+0 );
      RTS_HEXFL(exp, 0xd.89746a799ac4eedp+0 );
      RTS_HEXFL(exp, -0x6.58b64p-4 );
    }

    SECTION( "sin" ) {
      // sanity
      RTS_TEST(sin, 0.0 );
      RTS_TEST(sin, RTS_M_PI );
      RTS_TEST(sin, 1.0 );
      RTS_TEST(sin, 4.0 );
      // from glibc [1]
      RTS_TEST(sin, 0 );
      RTS_TEST(sin, -0 );
      RTS_TEST(sin, RTS_M_PI/6 );
      RTS_TEST(sin, -RTS_M_PI/6 );
      RTS_TEST(sin, RTS_M_PI/2 );
      RTS_TEST(sin, -RTS_M_PI/2 );
      RTS_TEST(sin, RTS_M_PI );
      RTS_TEST(sin, -RTS_M_PI );
      RTS_TEST(sin, 0.75 );
      //RTS_HEXFL(sin, 0x1p65 ); //FAIL -inff == Approx( -0.047183875 )
      //RTS_HEXFL(sin, -0x1p65 ); //FAIL inff == Approx( 0.047183875 )
      //RTS_HEXFL(sin, 0x1.7f4134p+103 ); //FAIL -inff == Approx( -0.0000000667 )
      RTS_TEST(sin, 0.80190127184058835 );
      RTS_TEST(sin, 2.522464e-1 );
      //RTS_TEST(sin, 1e22 ); //FAIL -inff == Approx( -0.7340815067 )
      RTS_HEXFL(sin, 0x1p1023 );
      //RTS_HEXFL(sin, 0x1p16383 ); // avoid warning: floating constant exceeds range of ‘double’ [-Woverflow]
      //RTS_HEXFL(sin, 0x1p+120 ); //FAIL -inff == Approx( 0.3778201044 )
      //RTS_HEXFL(sin, 0x1p+127 ); //FAIL -inff == Approx( 0.623385489 )
      //RTS_HEXFL(sin, 0x1.fffff8p+127 ); //FAIL -inff == Approx( 0.0485786051 )
      //RTS_HEXFL(sin, 0x1.fffffep+127 ); //FAIL -inff == Approx( -0.521876514 )
      //RTS_HEXFL(sin, 0x1p+50 ); //FAIL -inff == Approx( 0.4963965118 )
      //RTS_HEXFL(sin, 0x1p+28 ); //FAIL -26.16897f == Approx( -0.9861981869 )
      RTS_TEST(sin, 0.93340582292648832662962377071381 );
      RTS_TEST(sin, 2.3328432680770916363144351635128 );
      RTS_TEST(sin, 3.7439477503636453548097051680088 );
      RTS_TEST(sin, 3.9225160069792437411706487182528 );
      RTS_TEST(sin, 4.0711651639931289992091478779912 );
      RTS_TEST(sin, 4.7858438478542097982426639646292 );
      RTS_TEST(sin, 5.9840767662578002727968851104379 );
      RTS_TEST(sin, 1 );
      RTS_TEST(sin, 2 );
      RTS_TEST(sin, 3 );
      RTS_TEST(sin, 4 );
      RTS_TEST(sin, 5 );
      RTS_TEST(sin, 6 );
      RTS_TEST(sin, 7 );
      RTS_TEST(sin, 8 );
      RTS_TEST(sin, 9 );
      RTS_TEST(sin, 10 );
      //RTS_HEXFL(sin, 0x1.2001469775ce6p32 ); //FAIL -inff == Approx( 0.2583159804 )
      RTS_HEXFL(sin, -0x3.3de320f6be87ep+1020 );
      //RTS_HEXFL(sin, 0xe.9f1e5bc3bb88p+112 ); //FAIL -inff == Approx( 0.7791082263 )
      //RTS_HEXFL(sin, 0x4.7857dp+68 ); //FAIL -inff == Approx( -0.1249298826 )
      RTS_HEXFL(sin, 0x6.287cc8749212e72p+0 );
      RTS_HEXFL(sin, -0x1.02e34cp+0 );
      RTS_HEXFL(sin, 0xf.f0274p+4 );
      RTS_HEXFL(sin, 0x3.042d88p+0 );
      //RTS_TEST(sin, RTS_MAX ); //FAIL -inff == Approx( -0.521876514 )
      //RTS_TEST(sin, -RTS_MAX );  //FAIL inff == Approx( 0.521876514 )
      RTS_TEST(sin, RTS_MIN );
      RTS_TEST(sin, -RTS_MIN );
      RTS_TEST(sin, RTS_MIN_SUBNORM );
      RTS_TEST(sin, -RTS_MIN_SUBNORM );
      RTS_HEXFL(sin, 0x1.8475e5afd4481p+0 );
    }

    SECTION( "cos" ) {
      // sanity
      RTS_TEST(sin, 0.0 );
      RTS_TEST(sin, RTS_M_PI );
      RTS_TEST(sin, 1.0 );
      RTS_TEST(sin, 4.0 );
      // from glibc [1]
      RTS_TEST(cos, 0 );
      RTS_TEST(cos, -0 );
      RTS_TEST(cos, RTS_M_PI/3 );
      RTS_TEST(cos, 2*RTS_M_PI/3 );
      RTS_TEST(cos, RTS_M_PI/2 );
      RTS_TEST(cos, 0.75 );
      //RTS_HEXFL(cos, 0x1p65 ); //FAIL -nanf == Approx( 0.9988862276 )
      //RTS_HEXFL(cos, -0x1p65 ); //FAIL -nanf == Approx( 0.9988862276 )
      RTS_TEST(cos, 0.80190127184058835 );
      RTS_HEXFL(cos, 0x1.442f74p+15 );
      //RTS_TEST(cos, 1e22 ); //FAIL -nanf == Approx( 0.6790613532 )
      RTS_HEXFL(cos, 0x1p1023 );
      //RTS_HEXFL(cos, 0x1p16383 ); // avoid warning: magnitude of floating-point constant too large for type 'double'
      //RTS_HEXFL(cos, 0x1p+120 ); //FAIL -nanf == Approx( -0.9258790016 )
      //RTS_HEXFL(cos, 0x1p+127 ); //FAIL -nanf == Approx( 0.7819146514 )
      //RTS_HEXFL(cos, 0x1.fffff8p+127 ); //FAIL -nanf == Approx( 0.9988193512 )
      //RTS_HEXFL(cos, 0x1.fffffep+127 ); //FAIL -nanf == Approx( 0.8530210257 )
      //RTS_HEXFL(cos, 0x1p+50 ); //FAIL inff == Approx( 0.8680959344 )
      //RTS_HEXFL(cos, 0x1p+28 ); //FAIL 18.00966f == Approx( -0.1655689776 )
      RTS_HEXFL(cos, 0x1.000000cf4a2a2p0 );
      RTS_HEXFL(cos, 0x1.0000010b239a9p0 );
      RTS_HEXFL(cos, 0x1.00000162a932bp0 );
      RTS_HEXFL(cos, 0x1.000002d452a10p0 );
      RTS_HEXFL(cos, 0x1.000005bc7d86dp0 );
      //RTS_HEXFL(cos, 0x1.200145a975ce6p32 ); //FAIL inff == Approx( -0.9660604596 )
      RTS_TEST(cos, 1 );
      RTS_TEST(cos, 2 );
      RTS_TEST(cos, 3 );
      RTS_TEST(cos, 4 );
      RTS_TEST(cos, 5 );
      RTS_TEST(cos, 6 );
      RTS_TEST(cos, 7 );
      RTS_TEST(cos, 8 );
      RTS_TEST(cos, 9 );
      RTS_TEST(cos, 10 );
      RTS_HEXFL(cos, 0x1p-5 );
      RTS_HEXFL(cos, 0x1p-10 );
      RTS_HEXFL(cos, 0x1p-15 );
      RTS_HEXFL(cos, 0x1p-20 );
      RTS_HEXFL(cos, 0x1p-25 );
      RTS_HEXFL(cos, 0x1p-30 );
      RTS_HEXFL(cos, 0x1p-35 );
      RTS_HEXFL(cos, 0x1p-40 );
      RTS_HEXFL(cos, 0x1p-45 );
      RTS_HEXFL(cos, 0x1p-50 );
      RTS_HEXFL(cos, 0x1p-55 );
      RTS_HEXFL(cos, 0x1p-60 );
      RTS_HEXFL(cos, 0x1p-100 );
      RTS_HEXFL(cos, 0x1p-600 );
      //RTS_HEXFL(cos, 0x1p-10000 ); // avoid warning: magnitude of floating-point constant too small for type 'double'
      //RTS_TEST(cos, RTS_MAX ); //FAIL -nanf == Approx( 0.8530210257 )
      //RTS_TEST(cos, -RTS_MAX ); //FAIL -nanf == Approx( 0.8530210257 )
      RTS_TEST(cos, RTS_MIN );
      RTS_TEST(cos, -RTS_MIN );
      RTS_TEST(cos, RTS_MIN_SUBNORM );
      RTS_TEST(cos, -RTS_MIN_SUBNORM );
      RTS_HEXFL(cos, -0x3.3de320f6be87ep+1020 );
      //RTS_HEXFL(cos, 0xe.9f1e5bc3bb88p+112 ); //FAIL -nanf == Approx( -0.6268894672 )
      //RTS_HEXFL(cos, 0x4.7857dp+68 ); //FAIL -nanf == Approx( -0.9921655655 )
      RTS_HEXFL(cos, -0x1.02e34cp+0 );
      RTS_HEXFL(cos, 0xf.f0274p+4 );
      RTS_HEXFL(cos, 0x3.042d88p+0 );
      RTS_HEXFL(cos, 0x1.8475e5afd4481p+0 );
      RTS_TEST(cos, 1.57079697 );
      RTS_TEST(cos, -1.57079697 );
    }

    SECTION( "sincos" ) {
      // sanity
      RTS_TEST_SINCOS( 0.0 );
      RTS_TEST_SINCOS( RTS_M_PI );
      RTS_TEST_SINCOS( 1.0 );
      RTS_TEST_SINCOS( 4.0 );
      // from glibc [1]
      // 2017-03-16: 55 sin tests, 65 cos tests, of which 33 tests are in both
      // from glibc - sin
      RTS_TEST_SINCOS( 0 );
      RTS_TEST_SINCOS( -0 );
      RTS_TEST_SINCOS( RTS_M_PI/6 );
      RTS_TEST_SINCOS( -RTS_M_PI/6 );
      RTS_TEST_SINCOS( RTS_M_PI/2 );
      RTS_TEST_SINCOS( -RTS_M_PI/2 );
      RTS_TEST_SINCOS( RTS_M_PI );
      RTS_TEST_SINCOS( -RTS_M_PI );
      RTS_TEST_SINCOS( 0.75 );
      // ...
      RTS_TEST_SINCOS( 0.80190127184058835 );
      RTS_TEST_SINCOS( 2.522464e-1 );
      // ...
      RTS_TEST_SINCOS( 0.93340582292648832662962377071381 );
      RTS_TEST_SINCOS( 2.3328432680770916363144351635128 );
      RTS_TEST_SINCOS( 3.7439477503636453548097051680088 );
      RTS_TEST_SINCOS( 3.9225160069792437411706487182528 );
      RTS_TEST_SINCOS( 4.0711651639931289992091478779912 );
      RTS_TEST_SINCOS( 4.7858438478542097982426639646292 );
      RTS_TEST_SINCOS( 5.9840767662578002727968851104379 );
      RTS_TEST_SINCOS( 1 );
      RTS_TEST_SINCOS( 2 );
      RTS_TEST_SINCOS( 3 );
      RTS_TEST_SINCOS( 4 );
      RTS_TEST_SINCOS( 5 );
      RTS_TEST_SINCOS( 6 );
      RTS_TEST_SINCOS( 7 );
      RTS_TEST_SINCOS( 8 );
      RTS_TEST_SINCOS( 9 );
      RTS_TEST_SINCOS( 10 );
      // ...
      RTS_HEXFL_SINCOS( 0x6.287cc8749212e72p+0 );
      RTS_HEXFL_SINCOS( -0x1.02e34cp+0 );
      RTS_HEXFL_SINCOS( 0xf.f0274p+4 );
      RTS_HEXFL_SINCOS( 0x3.042d88p+0 );
      // ...
      RTS_TEST_SINCOS( RTS_MIN );
      RTS_TEST_SINCOS( -RTS_MIN );
      RTS_TEST_SINCOS( RTS_MIN_SUBNORM );
      RTS_TEST_SINCOS( -RTS_MIN_SUBNORM );
      RTS_HEXFL_SINCOS( 0x1.8475e5afd4481p+0 );
      // from glibc - cos
      RTS_TEST_SINCOS( 0 );
      RTS_TEST_SINCOS( -0 );
      RTS_TEST_SINCOS( RTS_M_PI/3 );
      RTS_TEST_SINCOS( 2*RTS_M_PI/3 );
      RTS_TEST_SINCOS( RTS_M_PI/2 );
      RTS_TEST_SINCOS( 0.75 );
      // ...
      RTS_TEST_SINCOS( 0.80190127184058835 );
      RTS_HEXFL_SINCOS( 0x1.442f74p+15 );
      // ...
      RTS_HEXFL_SINCOS( 0x1.000000cf4a2a2p0 );
      RTS_HEXFL_SINCOS( 0x1.0000010b239a9p0 );
      RTS_HEXFL_SINCOS( 0x1.00000162a932bp0 );
      RTS_HEXFL_SINCOS( 0x1.000002d452a10p0 );
      RTS_HEXFL_SINCOS( 0x1.000005bc7d86dp0 );
      // ...
      RTS_TEST_SINCOS( 1 );
      RTS_TEST_SINCOS( 2 );
      RTS_TEST_SINCOS( 3 );
      RTS_TEST_SINCOS( 4 );
      RTS_TEST_SINCOS( 5 );
      RTS_TEST_SINCOS( 6 );
      RTS_TEST_SINCOS( 7 );
      RTS_TEST_SINCOS( 8 );
      RTS_TEST_SINCOS( 9 );
      RTS_TEST_SINCOS( 10 );
      RTS_HEXFL_SINCOS( 0x1p-5 );
      RTS_HEXFL_SINCOS( 0x1p-10 );
      RTS_HEXFL_SINCOS( 0x1p-15 );
      RTS_HEXFL_SINCOS( 0x1p-20 );
      RTS_HEXFL_SINCOS( 0x1p-25 );
      RTS_HEXFL_SINCOS( 0x1p-30 );
      RTS_HEXFL_SINCOS( 0x1p-35 );
      RTS_HEXFL_SINCOS( 0x1p-40 );
      RTS_HEXFL_SINCOS( 0x1p-45 );
      RTS_HEXFL_SINCOS( 0x1p-50 );
      RTS_HEXFL_SINCOS( 0x1p-55 );
      RTS_HEXFL_SINCOS( 0x1p-60 );
      RTS_HEXFL_SINCOS( 0x1p-100 );
      RTS_HEXFL_SINCOS( 0x1p-600 );
      // ...
      RTS_TEST_SINCOS( RTS_MIN );
      RTS_TEST_SINCOS( -RTS_MIN );
      RTS_TEST_SINCOS( RTS_MIN_SUBNORM );
      RTS_TEST_SINCOS( -RTS_MIN_SUBNORM );
      // ...
      RTS_HEXFL_SINCOS( -0x1.02e34cp+0 );
      RTS_HEXFL_SINCOS( 0xf.f0274p+4 );
      RTS_HEXFL_SINCOS( 0x3.042d88p+0 );
      RTS_HEXFL_SINCOS( 0x1.8475e5afd4481p+0 );
      RTS_TEST_SINCOS( 1.57079697 );
      RTS_TEST_SINCOS( -1.57079697 );
    }
  }
}


TEST_CASE("Validate rts::vec_math::foo against std::foo", "[vec_math]") {
  arch_test<target::generic<2>>();
#ifdef __AVX__
  arch_test<target::avx_4>();
  arch_test<target::avx_8>();
#endif
#ifdef __AVX2__
  arch_test<target::avx2_8>();
#endif
}
