#pragma once

namespace rts {

  namespace vec_intrinsics {

    #define RTS_OP_2(name,op,U) \
      RTS_STMT_2(name,result[i] = u[i] op v[i],U,U)
    #define RTS_EXPR_2(name,expr,U) \
      RTS_STMT_2(name,result[i] = expr,U,U)

    #define RTS_STMT_1(name,stmt,U) \
      RTS_STMT_DECL(name,(const rts::vec<U,A> & u),stmt,U)
    #define RTS_STMT_2(name,stmt,U,V) \
      RTS_STMT_DECL(name,(const rts::vec<U,A> & u, const rts::vec<V,A> & v),stmt,U)
    #define RTS_STMT_3(name,stmt,U,V,W) \
      RTS_STMT_DECL(name,(const rts::vec<U,A> & u, const rts::vec<V,A> & v, const rts::vec<W,A> & w),stmt,U)

    #define RTS_STMT_DECL(name,argl,stmt,R) \
      RTS_ALWAYS_INLINE RTS_MATH_PURE rts::vec<R,A> name argl  RTS_MATH_NOEXCEPT { \
        rts::vec<R,A> result; \
        for (int i=0;i<A::width;++i) { \
          stmt; \
        } \
        return result; \
      }

    #define RTS_BITS_UV(expr) \
      auto ui = *(std::int32_t*)&(u[i]); \
      auto vi = *(std::int32_t*)&(v[i]); \
      auto pr = (std::int32_t*)&(result[i]); \
      *pr = expr;
    #define RTS_BITS_U(expr) \
      auto ui = *(std::int32_t*)&(u[i]); \
      auto pr = (std::int32_t*)&(result[i]); \
      *pr = expr;
    #define RTS_BITS_V(expr) \
      auto vi = *(std::int32_t*)&(v[i]); \
      auto pr = (std::int32_t*)&(result[i]); \
      *pr = expr;

    #define RTS_128(name,fun,U) RTS_128_2(name,fun,U,U)
    #define RTS_256(name,fun,U) RTS_256_2(name,fun,U,U)
    #define RTS_256x(name,fun,U) RTS_256x_2(name,fun,U,U)

    #define RTS_128_1(name,fun,U) RTS_M_1(name,fun(u.m),U,target::avx_4)
    #define RTS_128_2(name,fun,U,V) RTS_M_2(name,fun(u.m,v.m),U,V,target::avx_4)
    #define RTS_128_3(name,fun,U,V,W) RTS_M_3(name,fun(u.m,v.m,w.m),U,V,W,target::avx_4)

    // avx_8
    #define RTS_256x_1(name,fun,U) RTS_M_1(name,fun(u.m),U,target::avx_8)
    #define RTS_256x_2(name,fun,U,V) RTS_M_2(name,fun(u.m,v.m),U,V,target::avx_8)
    #define RTS_256x_3(name,fun,U,V,W) RTS_M_3(name,fun(u.m,v.m,w.m),U,V,W,target::avx_8)

    #define RTS_2x128_1(name,fun,U) RTS_M_1(name,fun(u.m),U,target::avx_8)
    #define RTS_2x128_2(name,fun,U,V) RTS_M_2(name,fun(u.m,v.m),U,V,target::avx_8)
    #define RTS_2x128_3(name,fun,U,V,W) RTS_M_3(name,fun(u.m,v.m,w.m),U,V,W,target::avx_8)

    // avx2_8
    #define RTS_256_1(name,fun,U) RTS_M_1(name,fun(u.m),U,target::avx2_8)
    #define RTS_256_2(name,fun,U,V) RTS_M_2(name,fun(u.m,v.m),U,V,target::avx2_8)
    #define RTS_256_3(name,fun,U,V,W) RTS_M_3(name,fun(u.m,v.m,w.m),U,V,W,target::avx2_8)

    #define RTS_M_0(name,expr,R,arch) \
      RTS_ALWAYS_INLINE RTS_MATH_PURE rts::vec<R,arch> name (void) RTS_MATH_NOEXCEPT { \
        auto m = expr; \
        return rts::vec<R,arch>(m,rts::detail::internal_tag); \
      }
    #define RTS_M_1(name,expr,U,arch) \
      RTS_ALWAYS_INLINE RTS_MATH_PURE rts::vec<U,arch> name (const rts::vec<U,arch> & u) RTS_MATH_NOEXCEPT { \
        auto m = expr; \
        return rts::vec<U,arch>(m,rts::detail::internal_tag); \
      }
    #define RTS_M_2(name,expr,U,V,arch) \
      RTS_ALWAYS_INLINE RTS_MATH_PURE rts::vec<U,arch> name (const rts::vec<U,arch> & u, const rts::vec<V,arch> & v) RTS_MATH_NOEXCEPT { \
        auto m = expr; \
        return rts::vec<U,arch>(m,rts::detail::internal_tag); \
      }
    #define RTS_M_3(name,expr,U,V,W,arch) \
      RTS_ALWAYS_INLINE RTS_MATH_PURE rts::vec<U,arch> name (const rts::vec<U,arch> & u, const rts::vec<V,arch> & v, const rts::vec<W,arch> & w) RTS_MATH_NOEXCEPT { \
        auto m = expr; \
        return rts::vec<U,arch>(m,rts::detail::internal_tag); \
      }

    #define RTS_M_1R(name,expr,U,arch,R) \
      RTS_ALWAYS_INLINE RTS_MATH_PURE rts::vec<R,arch> name (const rts::vec<U,arch> & u) RTS_MATH_NOEXCEPT { \
        auto m = expr; \
        return rts::vec<R,arch>(m,rts::detail::internal_tag); \
      }


    // Arithmetics - add sub mul div

    template <class T, class A> RTS_OP_2(add,+,T)
    template <class T, class A> RTS_OP_2(sub,-,T)
    template <class T, class A> RTS_OP_2(mul,*,T)
    template <class T, class A> RTS_OP_2(div,/,T)
    #ifdef __AVX__
      template <> RTS_128(add,_mm_add_ps,float)
      template <> RTS_128(sub,_mm_sub_ps,float)
      template <> RTS_128(mul,_mm_mul_ps,float)
      template <> RTS_128(div,_mm_div_ps,float)
      template <> RTS_128(add,_mm_add_epi32,std::int32_t)
      template <> RTS_128(sub,_mm_sub_epi32,std::int32_t)
      template <> RTS_128(mul,_mm_mullo_epi32,std::int32_t)

      //  __AVX__: add_ps sub_ps mul_ps div_ps
      template <> RTS_256x(add,_mm256_add_ps,float)
      template <> RTS_256x(sub,_mm256_sub_ps,float)
      template <> RTS_256x(mul,_mm256_mul_ps,float)
      template <> RTS_256x(div,_mm256_div_ps,float)
      //  __AVX2__: add_epi32 sub_epi32 mullo_epi32
      // we take implementation from + operator
      //template <> RTS_256x(add,_mm256_add_epi32,std::int32_t)
      //template <> RTS_256x(sub,_mm256_sub_epi32,std::int32_t)
      //template <> RTS_256x(mul,_mm256_mullo_epi32,std::int32_t)

    #endif // __AVX__
    #ifdef __AVX2__
      //  __AVX__: add_ps sub_ps mul_ps div_ps
      template <> RTS_256(add,_mm256_add_ps,float)
      template <> RTS_256(sub,_mm256_sub_ps,float)
      template <> RTS_256(mul,_mm256_mul_ps,float)
      template <> RTS_256(div,_mm256_div_ps,float)
      //  __AVX2__: add_epi32 sub_epi32 mullo_epi32
      template <> RTS_256(add,_mm256_add_epi32,std::int32_t)
      template <> RTS_256(sub,_mm256_sub_epi32,std::int32_t)
      template <> RTS_256(mul,_mm256_mullo_epi32,std::int32_t)
    #endif // __AVX2__


    // Arithmetics - fmadd

    template <class T, class A> RTS_STMT_3(fmadd, result[i] = u[i] * v[i] + w[i], T,T,T)
    #ifdef __FMA__
      template <> RTS_128_3(fmadd,_mm_fmadd_ps,float,float,float)
      template <> RTS_256x_3(fmadd,_mm256_fmadd_ps,float,float,float)
      template <> RTS_256_3(fmadd,_mm256_fmadd_ps,float,float,float)
    #endif // __FMA__


    // Arithmetics - min max

    template <class T, class A> RTS_EXPR_2(max,std::max(u[i],v[i]),T)
    template <class T, class A> RTS_EXPR_2(min,std::min(u[i],v[i]),T)
    #ifdef __AVX__
      template <> RTS_128(max,_mm_max_ps,float)
      template <> RTS_128(min,_mm_min_ps,float)
      template <> RTS_128(max,_mm_max_epi32,std::int32_t)
      template <> RTS_128(min,_mm_min_epi32,std::int32_t)
    #endif // __AVX__
    #ifdef __AVX2__
      // __AVX__: max_ps min_ps
      template <> RTS_256(max,_mm256_max_ps,float)
      template <> RTS_256(min,_mm256_min_ps,float)
      // __AVX2__: max_epi32 min_epi32
      template <> RTS_256(max,_mm256_max_epi32,std::int32_t)
      template <> RTS_256(min,_mm256_min_epi32,std::int32_t)
    #endif // __AVX2__


    // Bit manipulations - andnot and_ xor_ or_

    template <class T, class A> RTS_EXPR_2(andnot,~u[i] & v[i],T)
    template <class T, class A> RTS_OP_2(and_,&,T)
    template <class T, class A> RTS_OP_2(xor_,^,T)
    template <class T, class A> RTS_OP_2(or_,|,T)

    template <class A> RTS_STMT_2(andnot,RTS_BITS_UV(~ui & vi),float,float)
    template <class A> RTS_STMT_2(and_,RTS_BITS_UV(ui & vi),float,float)
    template <class A> RTS_STMT_2(xor_,RTS_BITS_UV(ui ^ vi),float,float)
    template <class A> RTS_STMT_2(or_,RTS_BITS_UV(ui | vi),float,float)

    #ifdef __AVX__
      template <> RTS_128(andnot,_mm_andnot_ps,float)
      template <> RTS_128(and_,_mm_and_ps,float)
      template <> RTS_128(xor_,_mm_xor_ps,float)
      template <> RTS_128(or_,_mm_or_ps,float)
      template <> RTS_128(andnot,_mm_andnot_si128,std::int32_t)
      template <> RTS_128(and_,_mm_and_si128,std::int32_t)
      template <> RTS_128(xor_,_mm_xor_si128,std::int32_t)
      template <> RTS_128(or_,_mm_or_si128,std::int32_t)

      // __AVX__: andnot_ps and_ps xor_ps or_ps
      template <> RTS_256x(andnot,_mm256_andnot_ps,float)
      template <> RTS_256x(and_,_mm256_and_ps,float)
      template <> RTS_256x(xor_,_mm256_xor_ps,float)
      template <> RTS_256x(or_,_mm256_or_ps,float)
      // not avilable (should we use coerce)
      //template <> RTS_256x(andnot,_mm256_andnot_si256,std::int32_t)
      //template <> RTS_256x(and_,_mm256_and_si256,std::int32_t)
      //template <> RTS_256x(xor_,_mm256_xor_si256,std::int32_t)
      //template <> RTS_256x(or_,_mm256_or_si256,std::int32_t)
    #endif // __AVX__
    #ifdef __AVX2__
      // __AVX__: andnot_ps and_ps xor_ps or_ps
      template <> RTS_256(andnot,_mm256_andnot_ps,float)
      template <> RTS_256(and_,_mm256_and_ps,float)
      template <> RTS_256(xor_,_mm256_xor_ps,float)
      template <> RTS_256(or_,_mm256_or_ps,float)
      // __AVX2__: andnot_si256 and_si256 xor_si256 or_si256
      template <> RTS_256(andnot,_mm256_andnot_si256,std::int32_t)
      template <> RTS_256(and_,_mm256_and_si256,std::int32_t)
      template <> RTS_256(xor_,_mm256_xor_si256,std::int32_t)
      template <> RTS_256(or_,_mm256_or_si256,std::int32_t)
    #endif // __AVX2__

    // float,int
    template <class A> RTS_STMT_2(andnot,RTS_BITS_U(~ui & v[i]),float,std::int32_t)
    template <class A> RTS_STMT_2(andnot,RTS_BITS_U(~ui & v[i]),float,std::uint32_t)
    template <class A> RTS_STMT_2(and_,RTS_BITS_U(ui & v[i]),float,std::int32_t)
    template <class A> RTS_STMT_2(and_,RTS_BITS_U(ui & v[i]),float,std::uint32_t)
    template <class A> RTS_STMT_2(xor_,RTS_BITS_U(ui ^ v[i]),float,std::int32_t)
    template <class A> RTS_STMT_2(xor_,RTS_BITS_U(ui ^ v[i]),float,std::uint32_t)
    template <class A> RTS_STMT_2(or_,RTS_BITS_U(ui | v[i]),float,std::int32_t)
    template <class A> RTS_STMT_2(or_,RTS_BITS_U(ui | v[i]),float,std::uint32_t)
    // int,float
    template <class A> RTS_STMT_2(andnot,RTS_BITS_V(~u[i] & vi),std::int32_t,float)
    template <class A> RTS_STMT_2(andnot,RTS_BITS_V(~u[i] & vi),std::uint32_t,float)
    template <class A> RTS_STMT_2(and_,RTS_BITS_V(u[i] & vi),std::int32_t,float)
    template <class A> RTS_STMT_2(and_,RTS_BITS_V(u[i] & vi),std::uint32_t,float)
    template <class A> RTS_STMT_2(xor_,RTS_BITS_V(u[i] ^ vi),std::int32_t,float)
    template <class A> RTS_STMT_2(xor_,RTS_BITS_V(u[i] ^ vi),std::uint32_t,float)
    template <class A> RTS_STMT_2(or_,RTS_BITS_V(u[i] | vi),std::int32_t,float)
    template <class A> RTS_STMT_2(or_,RTS_BITS_V(u[i] | vi),std::uint32_t,float)

    #ifdef __AVX__
      // float,int
      template <> RTS_M_2(andnot,_mm_castsi128_ps(_mm_andnot_si128(_mm_castps_si128(u.m),v.m)),float,std::int32_t,target::avx_4)
      template <> RTS_M_2(and_,_mm_castsi128_ps(_mm_and_si128(_mm_castps_si128(u.m),v.m)),float,std::int32_t,target::avx_4)
      template <> RTS_M_2(xor_,_mm_castsi128_ps(_mm_xor_si128(_mm_castps_si128(u.m),v.m)),float,std::int32_t,target::avx_4)
      template <> RTS_M_2(or_,_mm_castsi128_ps(_mm_or_si128(_mm_castps_si128(u.m),v.m)),float,std::int32_t,target::avx_4)
      // int,float
      template <> RTS_M_2(andnot,_mm_andnot_si128(u.m,_mm_castps_si128(v.m)),std::int32_t,float,target::avx_4)
      template <> RTS_M_2(and_,_mm_and_si128(u.m,_mm_castps_si128(v.m)),std::int32_t,float,target::avx_4)
      template <> RTS_M_2(xor_,_mm_xor_si128(u.m,_mm_castps_si128(v.m)),std::int32_t,float,target::avx_4)
      template <> RTS_M_2(or_,_mm_or_si128(u.m,_mm_castps_si128(v.m)),std::int32_t,float,target::avx_4)

      //avx8
      // float,int
      template <> RTS_M_2(andnot,_mm256_andnot_ps(u.m,_mm256_castsi256_ps(v.m)),float,std::int32_t,target::avx_8)
      template <> RTS_M_2(and_,_mm256_and_ps(u.m,_mm256_castsi256_ps(v.m)),float,std::int32_t,target::avx_8)
      template <> RTS_M_2(xor_,_mm256_xor_ps(u.m,_mm256_castsi256_ps(v.m)),float,std::int32_t,target::avx_8)
      template <> RTS_M_2(or_,_mm256_or_ps(u.m,_mm256_castsi256_ps(v.m)),float,std::int32_t,target::avx_8)
      // int,float 
      template <> RTS_M_2(andnot,_mm256_castps_si256(_mm256_andnot_ps(_mm256_castsi256_ps(u.m),v.m)),std::int32_t,float,target::avx_8)
      template <> RTS_M_2(and_,_mm256_castps_si256(_mm256_and_ps(_mm256_castsi256_ps(u.m),v.m)),std::int32_t,float,target::avx_8)
      template <> RTS_M_2(xor_,_mm256_castps_si256(_mm256_xor_ps(_mm256_castsi256_ps(u.m),v.m)),std::int32_t,float,target::avx_8)
      template <> RTS_M_2(or_,_mm256_castps_si256(_mm256_or_ps(_mm256_castsi256_ps(u.m),v.m)),std::int32_t,float,target::avx_8)
    #endif // __AVX__
    #ifdef __AVX2__
      // float,int
      template <> RTS_M_2(andnot,_mm256_castsi256_ps(_mm256_andnot_si256(_mm256_castps_si256(u.m),v.m)),float,std::int32_t,target::avx2_8)
      template <> RTS_M_2(and_,_mm256_castsi256_ps(_mm256_and_si256(_mm256_castps_si256(u.m),v.m)),float,std::int32_t,target::avx2_8)
      template <> RTS_M_2(xor_,_mm256_castsi256_ps(_mm256_xor_si256(_mm256_castps_si256(u.m),v.m)),float,std::int32_t,target::avx2_8)
      template <> RTS_M_2(or_,_mm256_castsi256_ps(_mm256_or_si256(_mm256_castps_si256(u.m),v.m)),float,std::int32_t,target::avx2_8)
      // int,float
      template <> RTS_M_2(andnot,_mm256_andnot_si256(u.m,_mm256_castps_si256(v.m)),std::int32_t,float,target::avx2_8)
      template <> RTS_M_2(and_,_mm256_and_si256(u.m,_mm256_castps_si256(v.m)),std::int32_t,float,target::avx2_8)
      template <> RTS_M_2(xor_,_mm256_xor_si256(u.m,_mm256_castps_si256(v.m)),std::int32_t,float,target::avx2_8)
      template <> RTS_M_2(or_,_mm256_or_si256(u.m,_mm256_castps_si256(v.m)),std::int32_t,float,target::avx2_8)
    #endif // __AVX2__


    // Rounding - floor

    template <class T, class A> RTS_STMT_1(floor,result[i] = std::floor(u[i]),T)
    #ifdef __AVX__
      template <> RTS_128_1(floor,_mm_floor_ps,float)
      template <> RTS_256x_1(floor,_mm256_floor_ps,float)
    #endif // __AVX__
    #ifdef __AVX2__
      // __AVX__: floor
      template <> RTS_256_1(floor,_mm256_floor_ps,float)
    #endif // __AVX2__


    // Reinterpret casts - cast

    template <class R, class U, class A>
    RTS_ALWAYS_INLINE RTS_MATH_PURE rts::vec<R,A> cast (const rts::vec<U,A> & u) RTS_MATH_NOEXCEPT {
      rts::vec<R,A> result;
      for (int i=0;i<A::width;++i) {
        union { U u; R r; } x = { u[i] };
        result[i] = x.r;
      }
      return result;
    }
    #ifdef __AVX__
      template<> RTS_M_1R(cast,_mm_castps_si128(u.m),float,target::avx_4,std::int32_t)
      template<> RTS_M_1R(cast,_mm_castsi128_ps(u.m),std::int32_t,target::avx_4,float)

      template<> RTS_M_1R(cast,_mm256_castps_si256(u.m),float,target::avx_8,std::int32_t)
      template<> RTS_M_1R(cast,_mm256_castsi256_ps(u.m),std::int32_t,target::avx_8,float)
    #endif // __AVX__
    #ifdef __AVX2__
      // __AVX__: cast*
      template<> RTS_M_1R(cast,_mm256_castps_si256(u.m),float,target::avx2_8,std::int32_t)
      template<> RTS_M_1R(cast,_mm256_castsi256_ps(u.m),std::int32_t,target::avx2_8,float)
    #endif // __AVX2__


    // Convertions - cvt

    template <class T, class T1, class A>
    RTS_ALWAYS_INLINE RTS_MATH_PURE rts::vec<T,A> cvt (const rts::vec<T1,A> & u) RTS_MATH_NOEXCEPT {
       rts::vec<T,A> result;
       for (int i=0;i<A::width;++i)
         result[i] = static_cast<T>(u[i]);
       return result;
    }
    #ifdef __AVX__
      template<> RTS_M_1R(cvt,_mm_cvttps_epi32(u.m),float,target::avx_4,std::int32_t)
      template<> RTS_M_1R(cvt,_mm_cvtepi32_ps(u.m),std::int32_t,target::avx_4,float)

      template<> RTS_M_1R(cvt,_mm256_cvttps_epi32(u.m),float,target::avx_8,std::int32_t)
      template<> RTS_M_1R(cvt,_mm256_cvtepi32_ps(u.m),std::int32_t,target::avx_8,float)
    #endif // __AVX__
    #ifdef __AVX2__
      template<> RTS_M_1R(cvt,_mm256_cvttps_epi32(u.m),float,target::avx2_8,std::int32_t)
      template<> RTS_M_1R(cvt,_mm256_cvtepi32_ps(u.m),std::int32_t,target::avx2_8,float)
    #endif // __AVX2__


    // Comparisons - cmp

    template <int imm8, class T, class A>
    RTS_ALWAYS_INLINE RTS_MATH_PURE auto _cmp_ps (const rts::vec<T,A> & u, const rts::vec<T,A> & v) RTS_MATH_NOEXCEPT {
       std::uint64_t truth = 0xFFFFFFFF; // _pd is 0xFFFFFFFFFFFFFFFF;
       rts::vec<T,A> result;
       for (int i=0;i<A::width;++i) {
         #define RTS_CMP(op) result[i] = u[i] op v[i] ? *(T*)&truth : 0; break
         switch (imm8) {
         //case _CMP_EQ_OQ: ;
         case _CMP_LT_OS: RTS_CMP(<);
         case _CMP_LE_OS: RTS_CMP(<=);
         //case _CMP_UNORD_Q: ;
         //case _CMP_NEQ_UQ: ;
         //case _CMP_NLT_US: ;
         //case _CMP_NLE_US: ;
         //case _CMP_ORD_Q: ;
         //case _CMP_EQ_UQ: ;
         //case _CMP_NGE_US: ;
         //case _CMP_NGT_US: ;
         //case _CMP_FALSE_OQ: ;
         //case _CMP_NEQ_OQ: ;
         case _CMP_GE_OS: RTS_CMP(>=);
         case _CMP_GT_OS: RTS_CMP(>);
         //case _CMP_TRUE_UQ: ;
         case _CMP_EQ_OS: RTS_CMP(==);
         //case _CMP_LT_OQ: ;
         //case _CMP_LE_OQ: ;
         //case _CMP_UNORD_S: ;
         //case _CMP_NEQ_US: ;
         //case _CMP_NLT_UQ: ;
         //case _CMP_NLE_UQ: ;
         //case _CMP_ORD_S: ;
         //case _CMP_EQ_US: ;
         //case _CMP_NGE_UQ: ;
         //case _CMP_NGT_UQ: ;
         //case _CMP_FALSE_OS: ;
         case _CMP_NEQ_OS: RTS_CMP(!=);
         //case _CMP_GE_OQ: ;
         //case _CMP_GT_OQ: ;
         //case _CMP_TRUE_US: ;
         #undef RTS_CMP
         };
       }
       return result;
    }
    template <int imm8, class A>
    RTS_ALWAYS_INLINE RTS_MATH_PURE rts::vec<float,A> cmp (const rts::vec<float,A> & u, const rts::vec<float,A> & v) RTS_MATH_NOEXCEPT {
      return _cmp_ps<imm8,float,A>(u,v);
    }
    #ifdef __AVX__
      template <int imm8> RTS_M_2(cmp,_mm_cmp_ps(u.m,v.m,imm8),float,float,target::avx_4)
      template <int imm8> RTS_M_2(cmp,_mm256_cmp_ps(u.m,v.m,imm8),float,float,target::avx_8)

    #endif // __AVX__
    #ifdef __AVX2__
      // __AVX__: cmp_ps
      template <int imm8> RTS_M_2(cmp,_mm256_cmp_ps(u.m,v.m,imm8),float,float,target::avx2_8)
    #endif // __AVX2__


    // Comparisons - cmpeq

    template <class A> RTS_EXPR_2(cmpeq,(u[i] == v[i] ? 0xFFFFFFFF : 0),float)
    template <class A> RTS_EXPR_2(cmpeq,(u[i] == v[i] ? 0xFFFFFFFF : 0),std::int32_t)
    #ifdef __AVX__
      template <> RTS_128(cmpeq,_mm_cmpeq_ps,float)
      template <> RTS_128(cmpeq,_mm_cmpeq_epi32,std::int32_t)
    #endif // __AVX__
    #ifdef __AVX2__
      // _mm256_cmpeq_ps doesn't exist.  Perhaps implement using cmp<_CMP_EQ_OS>.
      template <> RTS_256(cmpeq,_mm256_cmpeq_epi32,std::int32_t)
    #endif // __AVX2__


    // Zeroing - setzero

    template <class T, class A>
    RTS_ALWAYS_INLINE rts::vec<T,A> setzero (void) RTS_MATH_NOEXCEPT {
      return rts::vec<T,A>(0);
    }
    #ifdef __AVX__
      template <> RTS_M_0(setzero,_mm_setzero_ps(),float,target::avx_4)
      template <> RTS_M_0(setzero,_mm_setzero_si128(),std::int32_t,target::avx_4)

      template <> RTS_M_0(setzero,_mm256_setzero_ps(),float,target::avx_8)
      template <> RTS_M_0(setzero,_mm256_setzero_si256(),std::int32_t,target::avx_8)
    #endif // __AVX__
    #ifdef __AVX2__
      template <> RTS_M_0(setzero,_mm256_setzero_ps(),float,target::avx2_8)
      template <> RTS_M_0(setzero,_mm256_setzero_si256(),std::int32_t,target::avx2_8)
    #endif // __AVX2__

  } // namespace vec_intrinsics
} // namespace rts

#undef RTS_OP_2
#undef RTS_EXPR_2
#undef RTS_STMT_1
#undef RTS_STMT_2
#undef RTS_STMT_3
#undef RTS_STMT_DECL
#undef RTS_BITS_UV
#undef RTS_BITS_U
#undef RTS_BITS_V
#undef RTS_128
#undef RTS_128_1
#undef RTS_128_2
#undef RTS_128_3
#undef RTS_256
#undef RTS_256_1
#undef RTS_256_2
#undef RTS_256_3
#undef RTS_256x
#undef RTS_256x_1
#undef RTS_256x_2
#undef RTS_256x_3
#undef RTS_M_0
#undef RTS_M_1
#undef RTS_M_2
#undef RTS_M_3
#undef RTS_M_1R
