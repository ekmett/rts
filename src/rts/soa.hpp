#pragma once

#include <immintrin.h>
#include <array>
#include <cassert>
#include <cstdint>
#include <functional>
#include "vec.hpp"

namespace rts {

  namespace detail {
    template <class Base, std::size_t N, class A = default_isa>
    struct soa_expr {
      using vector = typename Base::vector;
      // auto?
      RTS_ALWAYS_INLINE RTS_PURE vector vget(int i) const noexcept { return static_cast<const Base*>(this)->vget(i); }
      RTS_ALWAYS_INLINE RTS_PURE vector vget(int i, const vec<bool,A> & mask) const noexcept { return static_cast<const Base*>(this)->vget(i, mask); }
      RTS_ALWAYS_INLINE RTS_CONST constexpr Base & operator () () noexcept { return *static_cast<Base*>(this); }
      RTS_ALWAYS_INLINE RTS_CONST constexpr const Base & operator () () const noexcept { return *static_cast<const Base*>(this); }
    };

    #define RTS_SOA_OP(name,op) \
      template <class S, std::size_t N, class A = default_isa> \
      struct soa_op_##name : public soa_expr<soa_op_##name<S,N,A>,N,A> { \
        using vector = decltype(op std::declval<typename S::vector>); \
        S base; \
        RTS_ALWAYS_INLINE constexpr soa_op_##name(const S & base) noexcept : base(base) {} \
        RTS_ALWAYS_INLINE soa_op_##name(S && base) noexcept : base(std::move(base)) {} \
        RTS_ALWAYS_INLINE RTS_PURE vector vget(int i) noexcept { return op base.vget(i); } \
        RTS_ALWAYS_INLINE RTS_PURE vector vget(int i, const vec<bool,A> & mask) noexcept { return op base.vget(i); } \
      }; \
      template <class S, std::size_t N, class A> \
      RTS_ALWAYS_INLINE RTS_PURE constexpr auto operator op (const soa_expr<S,N,A> & base) noexcept { \
        return soa_op_##name<S,N,A>(base()); \
      } \
      template <class S, std::size_t N, class A> \
      RTS_ALWAYS_INLINE auto operator op (soa_expr<S,N,A> && base) noexcept { \
        return soa_op_##name<S,N,A>(std::move(base())); \
      }

    RTS_SOA_OP(neg,-)
    RTS_SOA_OP(binary_not,~)
    RTS_SOA_OP(logical_not,!)
    RTS_SOA_OP(address,&)
    RTS_SOA_OP(deref,*)

    #undef RTS_SOA_OP

    #define RTS_SOA_BINOP(name,op) \
      template <class S, class T, std::size_t N, class A = default_isa> \
      struct soa_binop_##name : public soa_expr<soa_binop_##name<S,T,N,A>,N,A> { \
        using vector = decltype(std::declval<typename S::vector> op std::declval<typename T::vector>); \
        S lhs; \
        T rhs; \
        RTS_ALWAYS_INLINE constexpr soa_binop_##name(const S & lhs, const T & rhs) noexcept : lhs(lhs), rhs(rhs) {} \
        RTS_ALWAYS_INLINE soa_binop_##name(S && lhs, T && rhs) noexcept : lhs(std::move(lhs)), rhs(std::move(rhs)) {} \
        RTS_ALWAYS_INLINE RTS_PURE vector vget(int i) noexcept { return lhs.vget(i) op rhs.vget(i); } \
        RTS_ALWAYS_INLINE RTS_PURE vector vget(int i, const vec<bool,A> & mask) noexcept { return lhs.vget(i, mask) op rhs.vget(i, mask); } \
      }; \
      \
      template <class S, class T, std::size_t N, class A> \
      RTS_ALWAYS_INLINE RTS_PURE constexpr auto operator op (const soa_expr<S,N,A> & lhs, const soa_expr<T,N,A> & rhs) noexcept { \
        return soa_binop_##name<S,T,N,A>(lhs(), rhs()); \
      } \
      template <class S, class T, std::size_t N, class A> \
      RTS_ALWAYS_INLINE auto operator op (soa_expr<S,N,A> && lhs, soa_expr<T,N,A> && rhs) noexcept { \
        return soa_binop_##name<S,T,N,A>(lhs(), rhs()); \
      }

    RTS_SOA_BINOP(add,+)
    RTS_SOA_BINOP(mul,*)
    RTS_SOA_BINOP(sub,-)
    RTS_SOA_BINOP(div,/)
    RTS_SOA_BINOP(mod,%)
    RTS_SOA_BINOP(shl,<<)
    RTS_SOA_BINOP(shr,>>)
    RTS_SOA_BINOP(binary_and,&)
    RTS_SOA_BINOP(binary_or,|)
    RTS_SOA_BINOP(xor,^)
    RTS_SOA_BINOP(logical_and,&&)
    RTS_SOA_BINOP(logical_or,||)
    RTS_SOA_BINOP(lt,<)
    RTS_SOA_BINOP(le,<=)
    RTS_SOA_BINOP(eq,==)
    RTS_SOA_BINOP(ge,>=)
    RTS_SOA_BINOP(gt,>)
    RTS_SOA_BINOP(ne,!=)

    #undef RTS_SOA_BINOP
  }

  // array of structure of arrays layout
  template <class T, std::size_t N, class A = default_isa>
  struct alignas(A::alignment) soa : detail::soa_expr<soa<T,N,A>,N,vec<T,A>> {
    using arch = A;
    using vector = vec<T,A>;
    using iterator = typename vector::iterator;;
    using const_iterator = typename vector::const_iterator;
    using pointer = typename vector::pointer;
    using const_pointer = typename vector::const_pointer;
    using reference = typename vector::reference;
    using const_reference = typename vector::const_reference;

    static const std::size_t size = N;
    static const int vsize = (N+A::mask) >> A::shift;
    static const int wsize = N >> A::shift;
    vector data[vsize];

    static const bool needs_last = (N & A::mask) != 0;
    static RTS_PURE vec<bool,A> last_mask() noexcept { return vec<int,A>(N & ~ A::mask, detail::step_tag); }

    RTS_ALWAYS_INLINE constexpr soa() noexcept {}
    RTS_ALWAYS_INLINE constexpr soa(T t) noexcept {
      vector v(t); // promote to a vector type
      for (auto && r : data) { r = v; } // bulk initialize
    }
    RTS_ALWAYS_INLINE soa(soa && rhs) noexcept : data(std::move(rhs.data)) {} // move

    RTS_ALWAYS_INLINE constexpr soa(T values[N]) noexcept {
      iterator o = begin();
      // TODO: compute this in A::width strides?
      for (T * i = values, * i_max = values + N;i < i_max;++o,++i)
        *o = *i;
    }

    template <typename Base>
    RTS_ALWAYS_INLINE constexpr soa(const detail::soa_expr<Base,N,A> & rhs) noexcept {
      int i = 0;
      for (;i < (N >> A::shift); ++i)
        vput(i,rhs.vget(i));
      if (needs_last)
        vput(i,rhs.vget(i,last_mask()));
    }

    RTS_ALWAYS_INLINE RTS_CONST constexpr iterator begin() noexcept { return data[0].begin(); }
    RTS_ALWAYS_INLINE RTS_CONST constexpr iterator end() noexcept { return begin() + N; }
    RTS_ALWAYS_INLINE RTS_CONST constexpr const_iterator begin() const noexcept { return data[0].cbegin(); }
    RTS_ALWAYS_INLINE RTS_CONST constexpr const_iterator end() const noexcept { return cbegin() + N; }
    RTS_ALWAYS_INLINE RTS_CONST constexpr const_iterator cbegin() const noexcept { return data[0].cbegin(); }
    RTS_ALWAYS_INLINE RTS_CONST constexpr const_iterator cend() const noexcept { return cbegin() + N; }
    RTS_ALWAYS_INLINE RTS_CONST constexpr operator pointer() noexcept { return begin(); }
    RTS_ALWAYS_INLINE RTS_CONST constexpr operator const_pointer () const noexcept { return cbegin(); }
    RTS_ALWAYS_INLINE void swap(soa & rhs) noexcept { std::swap(data,rhs.data); }
    RTS_ALWAYS_INLINE RTS_CONST constexpr vector & vget(int i) const noexcept { return data[i]; }
    RTS_ALWAYS_INLINE RTS_CONST constexpr const vector & vget(int i, const vec<bool,A> &) const noexcept { return data[i]; }
    RTS_ALWAYS_INLINE void vput(int i, const vector & v) noexcept { data[i] = v; }
    RTS_ALWAYS_INLINE RTS_CONST constexpr auto get(int i) noexcept { return data[i >> A::shift].get(i & A::shift_mask); }
    RTS_ALWAYS_INLINE RTS_CONST constexpr auto get(int i) const noexcept { return data[i >> A::shift].get(i & A::shift_mask); }
    RTS_ALWAYS_INLINE void put(int i, const T & v) noexcept { data[i >> A::shift].put(i & A::shift_mask, v); }

    #define RTS_SOA_ASSIGN(op) \
      template <typename Base> \
      RTS_ALWAYS_INLINE soa & operator op (const detail::soa_expr<Base,N,A> & rhs) noexcept { \
        int i = 0; \
        for (;i < (N >> A::shift); ++i) \
          data[i] op rhs.vget(i); \
        if (needs_last) \
          data[i] op rhs.vget(i,last_mask()); \
        return *this; \
      }

    RTS_SOA_ASSIGN(=)
    RTS_SOA_ASSIGN(+=)
    RTS_SOA_ASSIGN(-=)
    RTS_SOA_ASSIGN(*=)
    RTS_SOA_ASSIGN(/=)
    RTS_SOA_ASSIGN(%=)
    RTS_SOA_ASSIGN(<<=)
    RTS_SOA_ASSIGN(>>=)
    RTS_SOA_ASSIGN(&=)
    RTS_SOA_ASSIGN(|=)
    RTS_SOA_ASSIGN(^=)

    #undef RTS_SOA_ASSIGN

    template <typename Base>
    RTS_ALWAYS_INLINE soa & operator = (soa && rhs) noexcept {
      for (int i = 0;i < ((N + A :: shift_mask) >> A::shift); ++i)
        data[i] = std::move(rhs.data[i]);
    }

    RTS_ALWAYS_INLINE soa & operator ++ () noexcept {
      for (auto && r : data) ++r;
      return *this;
    }
    RTS_ALWAYS_INLINE soa operator ++ (int) noexcept {
      soa t(*this);
      operator++();
      return t;
    }
    RTS_ALWAYS_INLINE soa & operator -- () noexcept {
      for (auto && r : data) --r;
      return *this;
    }
    RTS_ALWAYS_INLINE soa operator -- (int) noexcept {
      soa t(*this);
      operator--();
      return t;
    }
  };

  template <std::size_t i, class T, std::size_t N, class A, class U>
  RTS_ALWAYS_INLINE auto put(soa<T,N,A> & v, U u) noexcept {
    static_assert(i<N,"index out of bounds");
    return v.put(i,u);
  }

  // ------------------------------------------------------------------------------------
  // Lift <cmath> into soa<>
  // ------------------------------------------------------------------------------------

  #define RTS_UNARY_MATH(fun) \
    namespace detail { \
      template <class S, std::size_t N, class A = default_isa> \
      struct soa_unary_math_##fun : public soa_expr<soa_unary_math_##fun<S,N,A>,N,A> { \
        using vector = decltype(fun(std::declval<typename S::vector>)); \
        S base; \
        template <class X> \
        RTS_ALWAYS_INLINE soa_unary_math_##fun(X && base) noexcept : base(std::forward(base)) {} \
        RTS_ALWAYS_INLINE RTS_MATH_PURE constexpr vector vget(int i) RTS_MATH_NOEXCEPT { return fun(base.vget(i)); } \
        RTS_ALWAYS_INLINE RTS_MATH_PURE constexpr vector vget(int i, const vec<bool,A> & mask) RTS_MATH_NOEXCEPT { return fun(base.vget(i)); } \
      }; \
    } \
    template <class S, std::size_t N, class A> \
    RTS_ALWAYS_INLINE RTS_PURE constexpr auto fun(const detail::soa_expr<S,N,A> & base) noexcept { \
      return detail::soa_unary_math_##fun<S,N,A>(base()); \
    } \
    template <class S, std::size_t N, class A> \
    RTS_ALWAYS_INLINE auto fun(detail::soa_expr<S,N,A> && base) noexcept { \
      return detail::soa_unary_math_##fun<S,N,A>(std::move(base())); \
    }

  #define RTS_BINARY_MATH(fun) \
    namespace detail { \
      template <class S, class T, std::size_t N, class A = default_isa> \
      struct soa_binary_math_##fun : public soa_expr<soa_binary_math_##fun<S,T,N,A>,N,A> { \
        using vector = decltype(fun(std::declval<typename S::vector>,std::declval<typename T::vector>)); \
        S lhs; \
        T rhs; \
        template <class X, class Y> \
        RTS_ALWAYS_INLINE constexpr soa_binary_math_##fun(X && lhs, Y && rhs) noexcept : lhs(std::forward(lhs)), rhs(std::forward(rhs)) {} \
        RTS_ALWAYS_INLINE RTS_MATH_PURE constexpr vector vget(int i) RTS_MATH_NOEXCEPT { return fun(lhs.vget(i),rhs.vget(i)); } \
        RTS_ALWAYS_INLINE RTS_MATH_PURE constexpr vector vget(int i, const vec<bool,A> & mask) RTS_MATH_NOEXCEPT { return fun(lhs.vget(i, mask),rhs.vget(i, mask)); } \
      }; \
    } \
    template <class S, class T, std::size_t N, class A> \
    RTS_ALWAYS_INLINE RTS_PURE constexpr auto fun(const detail::soa_expr<S,N,A> & lhs, const detail::soa_expr<T,N,A> & rhs) noexcept { \
      return detail::soa_binary_math_##fun<S,T,N,A>(lhs(), rhs()); \
    } \
    template <class S, class T, std::size_t N, class A> \
    RTS_ALWAYS_INLINE auto fun(detail::soa_expr<S,N,A> && lhs, detail::soa_expr<T,N,A> && rhs) noexcept { \
      return detail::soa_binary_math_##fun<S,T,N,A>(lhs(), rhs()); \
    }

  #define RTS_TERNARY_MATH(fun) \
    namespace detail { \
      template <class S, class T, class U, std::size_t N, class A = default_isa> \
      struct soa_ternary_math_##fun : public soa_expr<soa_ternary_math_##fun<S,T,U,N,A>,N,A> { \
        using vector = decltype(fun(std::declval<typename S::vector>,std::declval<typename T::vector>)); \
        S s; \
        T t; \
        U u; \
        template <class X, class Y, class Z>  \
        RTS_ALWAYS_INLINE constexpr soa_ternary_math_##fun(X && s, Y && t, Z && u) noexcept : s(std::forward(s)), t(std::forward(t)), u(std::forward(u)) {} \
        RTS_ALWAYS_INLINE RTS_MATH_PURE constexpr vector vget(int i) RTS_MATH_NOEXCEPT { return fun(s.vget(i),t.vget(i),u.vget(i)); } \
        RTS_ALWAYS_INLINE RTS_MATH_PURE constexpr vector vget(int i, const vec<bool,A> & mask) RTS_MATH_NOEXCEPT { return fun(s.vget(i, mask),t.vget(i, mask),u.vget(i, mask)); } \
      }; \
    } \
    template <class S, class T, class U, std::size_t N, class A> \
    RTS_ALWAYS_INLINE RTS_PURE constexpr auto fun(const detail::soa_expr<S,N,A> & s, const detail::soa_expr<T,N,A> & t, const detail::soa_expr<U,N,A> & u) noexcept { \
      return detail::soa_ternary_math_##fun<S,T,U,N,A>(s(), t(), u()); \
    } \
    template <class S, class T, class U, std::size_t N, class A> \
    RTS_ALWAYS_INLINE auto fun(detail::soa_expr<S,N,A> && s, detail::soa_expr<T,N,A> && t, detail::soa_expr<U,N,A> && u) noexcept { \
      return detail::soa_ternary_math_##fun<S,T,U,N,A>(s(), t(), u()); \
    }


  #include "x-math.hpp"
} // namespace rts

namespace std {
  template <class T, std::size_t N, class A>
  struct tuple_size<rts::soa<T,N,A>> : public integral_constant<size_t, N> {};

  template <std::size_t I, class T, std::size_t N, class A>
  struct tuple_element<I, rts::soa<T,N,A> > {
    using type = T;
  };

  template <std::size_t i, class T, std::size_t N, class A>
  RTS_ALWAYS_INLINE RTS_PURE auto get(rts::soa<T,N,A> & v) noexcept {
    static_assert(i<N,"index out of bounds");
    return v.get(i);
  }

  template <std::size_t i, class T, std::size_t N, class A>
  RTS_ALWAYS_INLINE RTS_PURE auto get(const rts::soa<T,N,A> & v) noexcept {
    static_assert(i<N,"index out of bounds");
    return v.get(i);
  }
} // namespace std