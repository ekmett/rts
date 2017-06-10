#pragma once

namespace rts {

  namespace vec_math {

    #define RTS_SI_CONST(name,val) alignas(32) static const rts::vec<std::int32_t,A> name(val)
    #define RTS_PS_INT_CONST(name,val) alignas(32) static const rts::vec<float,A> name = mm::cast<float>(rts::vec<std::int32_t,A>(val))
    #define RTS_PS_CONST(name,val) alignas(32) static const rts::vec<float,A> name(val)

    /* natural logarithm computed for 8 simultaneous float
       return NaN for x <= 0
    */
    template <class A>
    inline RTS_MATH_PURE rts::vec<float,A> log (const rts::vec<float,A> & v) RTS_MATH_NOEXCEPT {
      namespace mm = rts::vec_intrinsics;

      RTS_SI_CONST(inv_mant_mask, ~0x7f800000);
      RTS_SI_CONST(si_0x7f, 0x7f);
      /* the smallest non denormalized float number */
      RTS_PS_INT_CONST(min_norm_pos,0x00800000);
      RTS_PS_CONST(one, 1.0f);
      RTS_PS_CONST(ps_0p5, 0.5f);
      RTS_PS_CONST(cephes_SQRTHF, 0.707106781186547524);
      RTS_PS_CONST(cephes_log_p0, 7.0376836292E-2);
      RTS_PS_CONST(cephes_log_p1, - 1.1514610310E-1);
      RTS_PS_CONST(cephes_log_p2, 1.1676998740E-1);
      RTS_PS_CONST(cephes_log_p3, - 1.2420140846E-1);
      RTS_PS_CONST(cephes_log_p4, + 1.4249322787E-1);
      RTS_PS_CONST(cephes_log_p5, - 1.6668057665E-1);
      RTS_PS_CONST(cephes_log_p6, + 2.0000714765E-1);
      RTS_PS_CONST(cephes_log_p7, - 2.4999993993E-1);
      RTS_PS_CONST(cephes_log_p8, + 3.3333331174E-1);
      RTS_PS_CONST(cephes_log_q1, -2.12194440e-4);
      RTS_PS_CONST(cephes_log_q2, 0.693359375);

      //__m256 invalid_mask = _mm256_cmple_ps(x, _mm256_setzero_ps());
      auto invalid_mask = mm::cast<std::int32_t>(mm::cmp<_CMP_LE_OS>(v, mm::setzero<float,A>()));

      auto x = mm::max(v, min_norm_pos); /* cut off denormalized stuff */

      auto imm0 = mm::cast<std::int32_t>(x) >> 23;

      /* keep only the fractional part */
      x = mm::and_(x, inv_mant_mask);
      x = mm::or_(x, ps_0p5);

      imm0 -= si_0x7f;
      auto e = mm::cvt<float>(imm0);
      e += one;

      /* part2:
         if( x < SQRTHF ) {
           e -= 1;
           x = x + x - 1.0;
         } else { x = x - 1.0; }
      */
      //__m256 mask = _mm256_cmplt_ps(x, *(__m256*)ps256_cephes_SQRTHF);
      auto mask = mm::cast<std::int32_t>(mm::cmp<_CMP_LT_OS>(x, cephes_SQRTHF));
      auto tmp = mm::and_(x, mask);
      x -= one;
      e -= mm::and_(one, mask);
      x += tmp;

      auto z = x * x;

      auto y = cephes_log_p0;
      y = mm::fmadd(y, x, cephes_log_p1);
      y = mm::fmadd(y, x, cephes_log_p2);
      y = mm::fmadd(y, x, cephes_log_p3);
      y = mm::fmadd(y, x, cephes_log_p4);
      y = mm::fmadd(y, x, cephes_log_p5);
      y = mm::fmadd(y, x, cephes_log_p6);
      y = mm::fmadd(y, x, cephes_log_p7);
      y = mm::fmadd(y, x, cephes_log_p8);
      y *= x;

      y *= z;

      y += e * cephes_log_q1;

      y -= z * ps_0p5;

      x += y;
      x += e * cephes_log_q2;
      x = mm::or_(x, invalid_mask); // negative arg will be NAN

      return x;

      // rts::vec<float,A> result;
      // for (int i=0;i<A::width;++i) result[i] = std::log(v[i]);
      // return result;
    }

    template <class A>
    inline RTS_MATH_PURE rts::vec<float,A> exp (const rts::vec<float,A> & v) RTS_MATH_NOEXCEPT {
      namespace mm = rts::vec_intrinsics;

      RTS_SI_CONST(si_0x7f, 0x7f);
      RTS_PS_CONST(one, 1.0f);
      RTS_PS_CONST(ps_0p5, 0.5f);

      RTS_PS_CONST(exp_hi, 88.3762626647949f);
      RTS_PS_CONST(exp_lo, -88.3762626647949f);

      RTS_PS_CONST(cephes_LOG2EF, 1.44269504088896341);
      RTS_PS_CONST(cephes_exp_C1, 0.693359375);
      RTS_PS_CONST(cephes_exp_C2, -2.12194440e-4);

      RTS_PS_CONST(cephes_exp_p0, 1.9875691500E-4);
      RTS_PS_CONST(cephes_exp_p1, 1.3981999507E-3);
      RTS_PS_CONST(cephes_exp_p2, 8.3334519073E-3);
      RTS_PS_CONST(cephes_exp_p3, 4.1665795894E-2);
      RTS_PS_CONST(cephes_exp_p4, 1.6666665459E-1);
      RTS_PS_CONST(cephes_exp_p5, 5.0000001201E-1);

      auto x = v;
      x = mm::min(x, exp_hi);
      x = mm::max(x, exp_lo);

      /* express exp(x) as exp(g + n*log(2)) */
      auto fx = mm::fmadd(x, cephes_LOG2EF, ps_0p5);

      /* how to perform a floorf with SSE: just below */
      //imm0 = _mm256_cvttps_epi32(fx);
      //tmp  = _mm256_cvtepi32_ps(imm0);

      auto tmp = mm::floor(fx);

      /* if greater, substract 1 */
      //__m256 mask = _mm256_cmpgt_ps(tmp, fx);
      auto mask = mm::cmp<_CMP_GT_OS>(tmp, fx);
      mask = mm::and_(mask, one);
      fx = tmp - mask;

      tmp = fx * cephes_exp_C1;
      auto z = fx * cephes_exp_C2;
      x -= tmp;
      x -= z;

      z = x * x;

      auto y = cephes_exp_p0;
      y = mm::fmadd(y, x, cephes_exp_p1);
      y = mm::fmadd(y, x, cephes_exp_p2);
      y = mm::fmadd(y, x, cephes_exp_p3);
      y = mm::fmadd(y, x, cephes_exp_p4);
      y = mm::fmadd(y, x, cephes_exp_p5);
      y = mm::fmadd(y, z, x);
      y += one;

      /* build 2^n */
      auto imm0 = mm::cvt<std::int32_t>(fx);
      // another two AVX2 instructions
      imm0 += si_0x7f; //int+
      imm0 <<= 23;
      auto pow2n = mm::cast<float>(imm0);
      y *= pow2n;

      return y;

      // rts::vec<float,A> result;
      // for (int i=0;i<A::width;++i) result[i] = std::exp(v[i]);
      // return result;
    }

    /* evaluation of 8 sines at onces using AVX intrisics

       The code is the exact rewriting of the cephes sinf function.
       Precision is excellent as long as x < 8192 (I did not bother to
       take into account the special handling they have for greater values
       -- it does not return garbage for arguments over 8192, though, but
       the extra precision is missing).

       Note that it is such that sinf((float)M_PI) = 8.74e-8, which is the
       surprising but correct result.

    */
    #define RTS_SIN_COS_DECLS \
      RTS_SI_CONST(si_0, 0); \
      RTS_SI_CONST(si_1, 1); \
      RTS_SI_CONST(si_inv1, ~1); \
      RTS_SI_CONST(si_2, 2); \
      RTS_SI_CONST(si_4, 4); \
      RTS_SI_CONST(si_0x7f, 0x7f); \
      RTS_PS_CONST(one, 1.0f);  \
      RTS_PS_CONST(ps_0p5, 0.5f); \
      RTS_PS_INT_CONST(sign_mask,(int)0x80000000); \
      RTS_PS_INT_CONST(inv_sign_mask,~0x80000000); \
      RTS_PS_CONST(minus_cephes_DP1, -0.78515625); \
      RTS_PS_CONST(minus_cephes_DP2, -2.4187564849853515625e-4); \
      RTS_PS_CONST(minus_cephes_DP3, -3.77489497744594108e-8); \
      RTS_PS_CONST(sincof_p0, -1.9515295891E-4); \
      RTS_PS_CONST(sincof_p1,  8.3321608736E-3); \
      RTS_PS_CONST(sincof_p2, -1.6666654611E-1); \
      RTS_PS_CONST(coscof_p0,  2.443315711809948E-005); \
      RTS_PS_CONST(coscof_p1, -1.388731625493765E-003); \
      RTS_PS_CONST(coscof_p2,  4.166664568298827E-002); \
      RTS_PS_CONST(cephes_FOPI, 1.27323954473516); // 4 / M_PI

    template <class A>
    inline RTS_MATH_PURE rts::vec<float,A> sin (const rts::vec<float,A> & v) RTS_MATH_NOEXCEPT {
      namespace mm = rts::vec_intrinsics;

      RTS_SIN_COS_DECLS

      /* take the absolute value */
      auto x = mm::and_(v, inv_sign_mask);
      /* extract the sign bit (upper one) */
      auto sign_bit = mm::and_(v, sign_mask);

      /* scale by 4/Pi */
      auto y0 = x * cephes_FOPI;

      /*
        Here we start a series of integer operations, which are in the
        realm of AVX2.
        If we don't have AVX, let's perform them using SSE2 directives
      */

      /* store the integer part of y in mm0 */
      auto imm2 = mm::cvt<std::int32_t>(y0);
      /* j=(j+1) & (~1) (see the cephes sources) */
      imm2 += si_1; //int+
      imm2 = mm::and_(imm2, si_inv1);
      y0 = mm::cvt<float>(imm2);

      /* get the swap sign flag */
      auto imm0 = mm::and_(imm2, si_4);
      imm0 <<= 29;
      /* get the polynom selection mask
         there is one polynom for 0 <= x <= Pi/4
         and another one for Pi/4<x<=Pi/2

         Both branches will be computed.
      */
      imm2 = mm::and_(imm2, si_2);
      imm2 = mm::cmpeq(imm2,si_0);

      auto swap_sign_bit = mm::cast<float>(imm0);
      auto poly_mask = mm::cast<float>(imm2);
      sign_bit = mm::xor_(sign_bit, swap_sign_bit);

      /* The magic pass: "Extended precision modular arithmetic"
         x = ((x - y * DP1) - y * DP2) - y * DP3; */
      x = mm::fmadd(y0, minus_cephes_DP1, x);
      x = mm::fmadd(y0, minus_cephes_DP2, x);
      x = mm::fmadd(y0, minus_cephes_DP3, x);

      /* Evaluate the first polynom  (0 <= x <= Pi/4) */
      auto y1 = coscof_p0;
      auto z = x * x;

      y1 = mm::fmadd(y1, z, coscof_p1);
      y1 = mm::fmadd(y1, z, coscof_p2);
      y1 *= z;
      y1 *= z;
      y1 -= z * ps_0p5;
      y1 += one;

      /* Evaluate the second polynom  (Pi/4 <= x <= 0) */

      auto y2 = sincof_p0;
      y2 = mm::fmadd(y2, z, sincof_p1);
      y2 = mm::fmadd(y2, z, sincof_p2);
      y2 *= z;
      y2 = mm::fmadd(y2, x, x);

      /* select the correct result from the two polynoms */
      auto xmm3 = poly_mask;
      y2 = mm::and_(xmm3, y2); //, xmm3);
      y1 = mm::andnot(xmm3, y1);
      y1 += y2;
      /* update the sign */
      y1 = mm::xor_(y1, sign_bit);

      return y1;

      // rts::vec<float,A> result;
      // for (int i=0;i<A::width;++i) result[i] = std::sin(v[i]);
      // return result;
    }

    /* almost the same as sin_ps */
    template <class A>
    inline RTS_MATH_PURE rts::vec<float,A> cos (const rts::vec<float,A> & v) RTS_MATH_NOEXCEPT {
      namespace mm = rts::vec_intrinsics;

      RTS_SIN_COS_DECLS

      /* take the absolute value */
      auto x = mm::and_(v, inv_sign_mask);

      /* scale by 4/Pi */
      auto y0 = x * cephes_FOPI;

      /* store the integer part of y in mm0 */
      auto imm2 = mm::cvt<std::int32_t>(y0);
      /* j=(j+1) & (~1) (see the cephes sources) */
      imm2 += si_1; //int+
      imm2 = mm::and_(imm2, si_inv1);
      y0 = mm::cvt<float>(imm2);
      imm2 -= si_2; //int-

      /* get the swap sign flag */
      auto imm0 = mm::andnot(imm2, si_4);
      imm0 <<= 29;
      /* get the polynom selection mask */
      imm2 = mm::and_(imm2, si_2);
      imm2 = mm::cmpeq(imm2, si_0);

      auto sign_bit = mm::cast<float>(imm0);
      auto poly_mask = mm::cast<float>(imm2);

      /* The magic pass: "Extended precision modular arithmetic"
         x = ((x - y * DP1) - y * DP2) - y * DP3; */
      x = mm::fmadd(y0, minus_cephes_DP1, x);
      x = mm::fmadd(y0, minus_cephes_DP2, x);
      x = mm::fmadd(y0, minus_cephes_DP3, x);

      /* Evaluate the first polynom  (0 <= x <= Pi/4) */
      auto y1 = coscof_p0;
      auto z = x * x;

      y1 = mm::fmadd(y1, z, coscof_p1);
      y1 = mm::fmadd(y1, z, coscof_p2);
      y1 *= z;
      y1 *= z;
      y1 -= z * ps_0p5;
      y1 += one;

      /* Evaluate the second polynom  (Pi/4 <= x <= 0) */

      auto y2 = sincof_p0;
      y2 = mm::fmadd(y2, z, sincof_p1);
      y2 = mm::fmadd(y2, z, sincof_p2);
      y2 *= z;
      y2 = mm::fmadd(y2, x, x);

      /* select the correct result from the two polynoms */
      auto xmm3 = poly_mask;
      y2 = mm::and_(xmm3, y2); //, xmm3);
      y1 = mm::andnot(xmm3, y1);
      y1 += y2;
      /* update the sign */
      y1 = mm::xor_(y1, sign_bit);

      return y1;

      // rts::vec<float,A> result;
      // for (int i=0;i<A::width;++i) result[i] = std::cos(v[i]);
      // return result;
    }

    /* since sin256_ps and cos256_ps are almost identical, sincos256_ps could replace both of them..
       it is almost as fast, and gives you a free cosine with your sine */
    template <class A>
    inline RTS_MATH_PURE void sincos (const rts::vec<float,A> & v, rts::vec<float,A> & rsin, rts::vec<float,A> & rcos) RTS_MATH_NOEXCEPT {
      namespace mm = rts::vec_intrinsics;

      RTS_SIN_COS_DECLS

      auto x = v;
      auto sign_bit_sin = x;
      /* take the absolute value */
      x = mm::and_(x, inv_sign_mask);
      /* extract the sign bit (upper one) */
      sign_bit_sin = mm::and_(sign_bit_sin, sign_mask);

      /* scale by 4/Pi */
      auto y0 = x * cephes_FOPI;

      /* store the integer part of y in imm2 */
      auto imm2 = mm::cvt<std::int32_t>(y0);

      /* j=(j+1) & (~1) (see the cephes sources) */
      imm2 += si_1; //int+
      imm2 = mm::and_(imm2, si_inv1);

      y0 = mm::cvt<float>(imm2);
      auto imm4 = imm2;

      /* get the swap sign flag for the sine */
      auto imm0 = mm::and_(imm2, si_4);
      imm0 <<= 29;
      //__m256 swap_sign_bit_sin = _mm256_castsi256_ps(imm0);

      /* get the polynom selection mask for the sine*/
      imm2 = mm::and_(imm2, si_2);
      imm2 = mm::cmpeq(imm2, si_0);
      //__m256 poly_mask = _mm256_castsi256_ps(imm2);

      auto swap_sign_bit_sin = mm::cast<float>(imm0);
      auto poly_mask = mm::cast<float>(imm2);

      /* The magic pass: "Extended precision modular arithmetic"
         x = ((x - y * DP1) - y * DP2) - y * DP3; */
      x = mm::fmadd(y0, minus_cephes_DP1, x);
      x = mm::fmadd(y0, minus_cephes_DP2, x);
      x = mm::fmadd(y0, minus_cephes_DP3, x);

      imm4 -= si_2; //int-
      imm4 = mm::andnot(imm4, si_4);
      imm4 <<= 29;

      auto sign_bit_cos = mm::cast<float>(imm4);

      sign_bit_sin = mm::xor_(sign_bit_sin, swap_sign_bit_sin);

      /* Evaluate the first polynom  (0 <= x <= Pi/4) */
      auto z = x * x;
      auto y1 = coscof_p0;

      y1 = mm::fmadd(y1, z, coscof_p1);
      y1 = mm::fmadd(y1, z, coscof_p2);
      y1 *= z;
      y1 *= z;
      y1 -= z * ps_0p5;
      y1 += one;

      /* Evaluate the second polynom  (Pi/4 <= x <= 0) */

      auto y2 = sincof_p0;
      y2 = mm::fmadd(y2, z, sincof_p1);
      y2 = mm::fmadd(y2, z, sincof_p2);
      y2 *= z;
      y2 = mm::fmadd(y2, x, x);

      /* select the correct result from the two polynoms */
      auto xmm3 = poly_mask;
      auto ysin2 = mm::and_(xmm3, y2);
      auto ysin1 = mm::andnot(xmm3, y1);
      y2 -= ysin2;
      y1 -= ysin1;

      auto xmm1 = ysin1 + ysin2;
      auto xmm2 = y1 + y2;

      /* update the sign */
      rsin = mm::xor_(xmm1, sign_bit_sin);
      rcos = mm::xor_(xmm2, sign_bit_cos);

      return;

      // rts::vec<float,A> result;
      // for (int i=0;i<A::width;++i) {
      //   rsin[i] = std::sin(v[i]);
      //   rcos[i] = std::cos(v[i]);
      // }
    }

  } // namespace vec_math
} // namespace rts

#undef RTS_SI_CONST
#undef RTS_PS_INT_CONST
#undef RTS_PS_CONST

/*
   AVX implementation of sin, cos, sincos, exp and log

   Based on "sse_mathfun.h", by Julien Pommier
   http://gruntthepeon.free.fr/ssemath/

   Copyright (C) 2012 Giovanni Garberoglio
   Interdisciplinary Laboratory for Computational Science (LISC)
   Fondazione Bruno Kessler and University of Trento
   via Sommarive, 18
   I-38123 Trento (Italy)

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.

  (this is the zlib license)
*/

// MODIFICATIONS FOR "SPMD" PROJECT
// 1. Remove v8sf/v8si/v4si typedefs (don't want to expose these in the interface)
// 2. Define ALIGN32_BEG/ALIGN32_END properly for MSVC
// 3. Explicitly cast 0x80000000 to int for sign_mask to silence warning.
// 4. Replace _mm256_and_si128 and _mm256_andnot_si128 with _mm256_and_si256 and _mm256_andnot_si256, respectively.
// 5. switch #warning to #pragma message for MSVC
// 6. rename functions from "_mm256*" to "mm256*", since MSVC doesn't want you to redefine intrinsics
// 7. Apply ALIGN32_BEG/ALIGN32_END to imm_xmm_union

// Modifications for yet another "SPMD" project by Edward Kmett
// 1. removed pre-avx2 support
// 2. converted to fit surrounding namespacing discipline mostly by prefixing defines and avoiding dangerous _ prefixes.
// 3. incorporated FMA3 since my oldest platform is Haswell

// Modifications for https://github.com/ekmett/rts
// 1. generalized to use a vector type with optional intrinsics
