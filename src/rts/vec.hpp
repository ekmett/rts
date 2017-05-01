#pragma once

#include <immintrin.h>
#include <array>
#include <cassert>
#include <cstdint>
#include <functional>
#include <tuple>
#include <utility>
#include "cpu.hpp"
#include "x86.hpp"

namespace rts {

  // helper for generic<N>
  namespace detail {
    static const struct internal_t {} internal_tag = {};
    static const struct indirection_t {} indirection_tag = {};
    static const struct step_t {} step_tag = {};
    static RTS_CONST constexpr int ilog2(int v) { // intended for constexpr use
      int r = 0;
      while (v >>= 1) ++r;
      return r;
    }
  }

  namespace target {
    // --------------------------------------------------------------------------------
    // * target instruction set architectures
    // --------------------------------------------------------------------------------

    // no explicit simd
    template <size_t N> 
    struct generic {
      static_assert(N <= 32, "vector width too wide");
      static_assert((N & (N-1)) == 0, "vector width is not a power of 2");
      static const int width = N;
      static const std::uint32_t width_mask = (1 << width) - 1;
      static const int shift = detail::ilog2(width);
      static const std::uint32_t shift_mask = (1 << shift) - 1;
      static const int alignment = 1;
      static const int allow_avx = false;
      static const int allow_avx2 = false;
      static const int allow_avx512 = false;
      static RTS_CONST constexpr bool available(isa i) noexcept { return true; }
    };

    #if defined(__AVX__)
      struct avx_4 {
        static const int width = 4;
        static const std::uint32_t width_mask = 0xf;
        static const int shift = 2;
        static const std::uint32_t shift_mask = 3;
        static const int alignment = 16;
        static const bool allow_avx = true;
        static const bool allow_avx2 = false;
        static const bool allow_avx512 = false;
        static RTS_CONST constexpr bool available(isa i) noexcept {
          return i >= isa::avx && i <= isa::max_intel;
        }
      };
    #endif

    #if defined(__AVX2__)
      struct avx2_8 {
        static const int width = 8;
        static const std::uint32_t width_mask = 0xff;
        static const int shift = 3;
        static const std::uint32_t shift_mask = 7;
        static const int alignment = 32;
        static const bool allow_avx = true;
        static const bool allow_avx2 = true;
        static const bool allow_avx512 = false;
        static RTS_CONST constexpr bool available(isa i) noexcept {
          return i >= isa::avx2 && i <= isa::max_intel;
        }
      };
    #endif

    #if defined(__AVX512F__)
      struct avx512_16 {
        static const int width = 16;
        static const std::uint32_t width_mask = 0xffff;
        static const int shift = 4;
        static const std::uint32_t shift_mask = 15;
        static const int alignment = 64;
        static const bool allow_avx = true;
        static const bool allow_avx2 = true;
        static const bool allow_avx512 = true;
        static RTS_CONST constexpr bool available(isa i) noexcept {
          return i >= isa::avx512_knl && i <= isa::max_intel;
        }
      };
    #endif

    // TODO: implement avx512_16
    #if defined(__AVX2__)
      using default_isa = avx2_8;
    #elif defined(__AVX__)
      using default_isa = avx_4;
    #else
      using default_isa = generic<1>;
    #endif

  } // arch

  using target::default_isa;

  // --------------------------------------------------------------------------------
  // * vec<T,A>
  // --------------------------------------------------------------------------------

  // primary template: reference implementation
  template <class T, class A = default_isa>
  struct vec {
    using arch = A;
    using value_type = T;
    using size_type = size_t;
    using difference_type = std::ptrdiff_t;
    using reference = T&;
    using const_reference = const T&;
    using pointer = T*;
    using const_pointer = const T*;
    using iterator = T*;
    using const_iterator = const T*;
    RTS_ALWAYS_INLINE RTS_CONST constexpr iterator begin() noexcept { return data; }
    RTS_ALWAYS_INLINE RTS_CONST constexpr iterator end() noexcept { return data + arch::width; }
    RTS_ALWAYS_INLINE RTS_CONST constexpr const_iterator begin() const noexcept { return data; }
    RTS_ALWAYS_INLINE RTS_CONST constexpr const_iterator end() const noexcept { return data + arch::width; }
    RTS_ALWAYS_INLINE RTS_CONST constexpr const_iterator cbegin() const noexcept { return data; }
    RTS_ALWAYS_INLINE RTS_CONST constexpr const_iterator cend() const noexcept { return data + arch::width;; }
    RTS_ALWAYS_INLINE RTS_CONST constexpr reference operator [] (int i) noexcept { return data[i]; }
    RTS_ALWAYS_INLINE RTS_CONST constexpr const_reference operator [] (int i) const noexcept { return data[i]; }

    T data [arch::width];

    RTS_ALWAYS_INLINE constexpr vec() noexcept = default;
    RTS_ALWAYS_INLINE constexpr vec(const vec & rhs) noexcept = default;
    RTS_ALWAYS_INLINE constexpr vec(T u) noexcept : data() { for (auto && r : data) r = u; } // only constexpr if we can loop in constexpr
    RTS_ALWAYS_INLINE constexpr vec(std::initializer_list<T> l) noexcept : data(l) {}
    RTS_ALWAYS_INLINE constexpr vec(const T & u, detail::step_t) noexcept {
      T v = u;
      for (auto && r : data) r = v++;
    }
    RTS_ALWAYS_INLINE vec(vec && rhs) noexcept = default;

    template <class U>
    RTS_ALWAYS_INLINE vec & operator=(vec<U,A> & rhs) noexcept {
      data = rhs.data;
      return *this;
    }

    RTS_ALWAYS_INLINE vec & operator=(const T & rhs) noexcept {
      data.fill(rhs);
      return *this;
    }

    RTS_ALWAYS_INLINE vec & operator=(vec<T,A> && rhs) noexcept {
      data = std::move(rhs.data);
      return *this;
    }

    RTS_ALWAYS_INLINE void swap(vec & rhs) noexcept {
      std::swap(data,rhs.data);
    }

    RTS_ALWAYS_INLINE RTS_CONST constexpr reference get(int i) noexcept { return data[i]; }
    RTS_ALWAYS_INLINE RTS_CONST constexpr const_reference get(int i) const noexcept { return data[i]; }
    RTS_ALWAYS_INLINE void put(int i, T rhs) noexcept { data[i] = rhs; }

    #define RTS_ASSIGN(op) \
      template <class U> \
      RTS_ALWAYS_INLINE vec & operator op (vec<U,A> & rhs) noexcept { \
        for (int i=0;i<A::width;++i) data[i] op rhs[i]; \
        return *this; \
      } \
      template <class U> \
      RTS_ALWAYS_INLINE vec & operator op (const U & rhs) noexcept { \
        for (int i=0;i<A::width;++i) data[i] op rhs; \
        return *this; \
      }
    RTS_ASSIGN(+=)
    RTS_ASSIGN(-=)
    RTS_ASSIGN(*=)
    RTS_ASSIGN(/=)
    RTS_ASSIGN(<<=)
    RTS_ASSIGN(>>=)
    RTS_ASSIGN(&=)
    RTS_ASSIGN(|=)
    RTS_ASSIGN(^=)
    #undef RTS_ASSIGN
  }; // vec<T,A>

  // --------------------------------------------------------------------------------
  // * forward declarations
  // --------------------------------------------------------------------------------

  namespace detail {
    template <class T, class A = default_isa> struct const_vref;
    template <class T, class A = default_isa> struct vref;
    template <class T, class A = default_isa> struct const_vptr;
    template <class T, class A = default_isa> struct vptr;

    template <class S, class T, class A> S & operator << (S & os, const vec<T,A> & v);
    template <class T, class A> RTS_ALWAYS_INLINE RTS_PURE constexpr const_vptr<T, A> operator + (std::ptrdiff_t j, const_vptr<T,A> & rhs) noexcept;
    template <class T, class A> RTS_ALWAYS_INLINE RTS_PURE constexpr const_vptr<T, A> operator + (const_vptr<T,A> & lhs, std::ptrdiff_t j) noexcept;
    template <class T, class A> RTS_ALWAYS_INLINE RTS_PURE constexpr vptr<T, A> operator+(std::ptrdiff_t j, vptr<T,A> & rhs) noexcept;
    template <class T, class A> RTS_ALWAYS_INLINE RTS_PURE constexpr vptr<T, A> operator+(vptr<T,A> & lhs, std::ptrdiff_t j) noexcept;
    template <class T, class A> RTS_ALWAYS_INLINE RTS_PURE constexpr std::ptrdiff_t operator - (const const_vptr<T,A> & lhs, const const_vptr<T,A> & rhs) noexcept;
    template <class T, class A> RTS_ALWAYS_INLINE RTS_PURE constexpr bool operator == (const const_vptr<T,A> & lhs, const const_vptr<T,A> & rhs) noexcept;
    template <class T, class A> RTS_ALWAYS_INLINE RTS_PURE constexpr bool operator == (const const_vptr<T,A> & lhs, std::nullptr_t) noexcept;
    template <class T, class A> RTS_ALWAYS_INLINE RTS_PURE constexpr bool operator == (std::nullptr_t, const const_vptr<T,A> & rhs) noexcept;
    template <class T, class A> RTS_ALWAYS_INLINE RTS_PURE constexpr bool operator != (const const_vptr<T,A> & lhs, const const_vptr<T,A> & rhs) noexcept;
    template <class T, class A> RTS_ALWAYS_INLINE RTS_PURE constexpr bool operator != (const const_vptr<T,A> & lhs, std::nullptr_t) noexcept;
    template <class T, class A> RTS_ALWAYS_INLINE RTS_PURE constexpr bool operator != (std::nullptr_t, const const_vptr<T,A> & rhs) noexcept;
    template <class T, class A> RTS_ALWAYS_INLINE RTS_PURE constexpr bool operator < (const const_vptr<T,A> & lhs, const const_vptr<T,A> & rhs) noexcept;
    template <class T, class A> RTS_ALWAYS_INLINE RTS_PURE constexpr bool operator < (std::nullptr_t, const const_vptr<T,A> & rhs) noexcept;
    template <class T, class A> RTS_ALWAYS_INLINE RTS_PURE constexpr bool operator < (const const_vptr<T,A> & lhs, std::nullptr_t) noexcept;
    template <class T, class A> RTS_ALWAYS_INLINE RTS_PURE constexpr bool operator <= (const const_vptr<T,A> & lhs, const const_vptr<T,A> & rhs) noexcept;
    template <class T, class A> RTS_ALWAYS_INLINE RTS_PURE constexpr bool operator <= (std::nullptr_t, const const_vptr<T,A> & rhs) noexcept;
    template <class T, class A> RTS_ALWAYS_INLINE RTS_PURE constexpr bool operator <= (const const_vptr<T,A> & lhs, std::nullptr_t) noexcept;
    template <class T, class A> RTS_ALWAYS_INLINE RTS_PURE constexpr bool operator > (const const_vptr<T,A> & lhs, const const_vptr<T,A> & rhs) noexcept;
    template <class T, class A> RTS_ALWAYS_INLINE RTS_PURE constexpr bool operator > (std::nullptr_t, const const_vptr<T,A> & rhs) noexcept;
    template <class T, class A> RTS_ALWAYS_INLINE RTS_PURE constexpr bool operator > (const const_vptr<T,A> & lhs, std::nullptr_t) noexcept;
    template <class T, class A> RTS_ALWAYS_INLINE RTS_PURE constexpr bool operator >= (const const_vptr<T,A> & lhs, const const_vptr<T,A> & rhs) noexcept;
    template <class T, class A> RTS_ALWAYS_INLINE RTS_PURE constexpr bool operator >= (std::nullptr_t, const const_vptr<T,A> & rhs) noexcept;
    template <class T, class A> RTS_ALWAYS_INLINE RTS_PURE constexpr bool operator >= (const const_vptr<T,A> & lhs, std::nullptr_t) noexcept;
    template <class T, class A> RTS_ALWAYS_INLINE const_vptr<T,A> & operator++(const_vptr<T,A> & lhs) noexcept;
    template <class T, class A> RTS_ALWAYS_INLINE const_vptr<T,A> & operator++(const_vptr<T,A> & lhs, int) noexcept;
    template <class T, class A> RTS_ALWAYS_INLINE const_vptr<T,A> & operator--(const_vptr<T,A> & lhs) noexcept;
    template <class T, class A> RTS_ALWAYS_INLINE const_vptr<T,A> & operator--(const_vptr<T,A> & lhs, int) noexcept;
    template <class T, class A> RTS_ALWAYS_INLINE vptr<T,A> & operator++(vptr<T,A> & lhs) noexcept;
    template <class T, class A> RTS_ALWAYS_INLINE vptr<T,A> & operator++(vptr<T,A> & lhs, int) noexcept;
    template <class T, class A> RTS_ALWAYS_INLINE vptr<T,A> & operator--(vptr<T,A> & lhs) noexcept;
    template <class T, class A> RTS_ALWAYS_INLINE vptr<T,A> & operator--(vptr<T,A> & lhs, int) noexcept;
    template <class T, class A> RTS_ALWAYS_INLINE RTS_CONST constexpr const_vref<T,A> make_const_vref(const vec<T,A> & v, std::int32_t i = 0) noexcept;


    // --------------------------------------------------------------------------------
    // * vector member references and pointers
    // --------------------------------------------------------------------------------

    template <class T, class A>
    struct const_vref {
      const vec<T, A> & v;
      const int i;

      const_vref() noexcept = delete;
      RTS_ALWAYS_INLINE constexpr const_vref(const const_vref & rhs) noexcept = default;
      RTS_ALWAYS_INLINE const_vref(const_vref && rhs) noexcept = default;
      RTS_ALWAYS_INLINE constexpr const_vref(const vec<T, A> & v) noexcept : v(v), i(0) {}    
      RTS_ALWAYS_INLINE constexpr const_vref(const vec<T, A> & v, int i) noexcept : v(v), i(i) {
        assert(0 <= i && i < A::width);
      }

      RTS_ALWAYS_INLINE RTS_PURE T get() const noexcept { return v.get(i); }
      RTS_ALWAYS_INLINE RTS_PURE operator T () const noexcept { return v.get(i); }
      RTS_ALWAYS_INLINE RTS_PURE constexpr const_vptr<T, A> operator & () const noexcept;
    };

    // (mutable) pointer to a constant element of a vec
    template <class T, class A>
    struct const_vptr {
      const vec<T, A> * v;
      std::int32_t i;

      RTS_ALWAYS_INLINE constexpr const_vptr() noexcept = default;
      RTS_ALWAYS_INLINE constexpr const_vptr(const const_vptr & rhs) noexcept = default;
      RTS_ALWAYS_INLINE const_vptr(const_vptr && rhs) noexcept = default;
      RTS_ALWAYS_INLINE constexpr const_vptr(std::nullptr_t) noexcept : v(nullptr), i(0) {}
      RTS_ALWAYS_INLINE constexpr const_vptr(const vec<T, A> * v, int i) noexcept : v(v), i(i) {
        assert(0 <= i && i < A::width);
      }

      RTS_ALWAYS_INLINE constexpr const_vptr(const vec<T, A> * v) noexcept : v(v), i(0) {}
      RTS_ALWAYS_INLINE const_vptr & operator = (const const_vptr & rhs) noexcept = default;
      RTS_ALWAYS_INLINE const_vptr & operator = (const_vptr && rhs) noexcept = default;
      RTS_ALWAYS_INLINE const_vptr & operator += (std::ptrdiff_t j) noexcept;
      RTS_ALWAYS_INLINE const_vptr & operator -= (std::ptrdiff_t j) noexcept;
      RTS_ALWAYS_INLINE RTS_PURE constexpr std::ptrdiff_t operator - (const const_vptr & that) const noexcept;
      RTS_ALWAYS_INLINE RTS_PURE constexpr const_vref<T,A> operator * () const noexcept { return make_const_vref(*v,i); }
      RTS_ALWAYS_INLINE RTS_PURE constexpr const_vref<T,A> operator [] (int j) const noexcept { return make_const_vref(*v,i + j); }

      RTS_ALWAYS_INLINE void swap(const_vptr & that) noexcept {
        std::swap(v,that.v);
        std::swap(i,that.i);
      }
    };

    template <class T, class A>
    RTS_ALWAYS_INLINE RTS_CONST constexpr const_vptr<T,A> make_const_vptr(const vec<T,A> * v, std::int32_t i = 0) noexcept {
      return const_vptr<T,A>(v,i);
    }

    template <class T, class A>
    RTS_ALWAYS_INLINE RTS_PURE constexpr const_vptr<T, A> const_vref<T,A>::operator & () const noexcept {
      return make_const_vptr(&v,i);
    }

    template <class T, class A>
    struct vref {
      vec<T, A> const & v;
      const std::int32_t i;

      vref() = delete;
      RTS_ALWAYS_INLINE constexpr vref(const vref & rhs) noexcept = default;
      RTS_ALWAYS_INLINE constexpr vref(vec<T, A> & v) noexcept: v(v), i(0) {}

      RTS_ALWAYS_INLINE constexpr vref(vec<T, A> & v, std::int32_t i = 0) noexcept: v(v), i(i) {
        assert(0 <= i && i < A::width);
      }

      template <typename U>
      RTS_ALWAYS_INLINE vref & operator = (U t) noexcept { v.put(i,t); }

      RTS_ALWAYS_INLINE RTS_PURE T get() const noexcept { return v.get(i); }
      RTS_ALWAYS_INLINE RTS_PURE operator T () const noexcept { return v.get(i); }
      RTS_ALWAYS_INLINE RTS_PURE constexpr operator const_vref<T,A> () const noexcept { return make_const_vref(v,i); }

      // forward
      RTS_ALWAYS_INLINE RTS_PURE constexpr vptr<T,A> operator & () const noexcept;
    };

    template <class T, class A>
    RTS_ALWAYS_INLINE constexpr vref<T,A> make_vref(vec<T,A> & v, std::int32_t i = 0) noexcept {
      return vref<T,A>(v,i);
    }

    template <class T, class A> 
    struct vptr {
      vec<T, A> * v;
      std::int32_t i;

      RTS_ALWAYS_INLINE constexpr vptr() noexcept = default;
      RTS_ALWAYS_INLINE constexpr vptr(std::nullptr_t) noexcept : v(nullptr), i(0) {}
      RTS_ALWAYS_INLINE constexpr vptr(const vptr & rhs) noexcept = default;
      RTS_ALWAYS_INLINE vptr(vptr && rhs) noexcept = default;

      RTS_ALWAYS_INLINE constexpr vptr(vec<T, A> * v, std::int32_t i = 0) noexcept : v(v), i(i) {
        assert(0 <= i && i < A::width); // invariant
      }

      RTS_ALWAYS_INLINE RTS_PURE constexpr vref<T, A> operator * () const noexcept {
        return make_vref(*v,i);
      }

      RTS_ALWAYS_INLINE RTS_PURE constexpr vref<T, A> operator [] (std::int32_t j) const noexcept {
        return make_vref(*v,i + j);
      }

      RTS_ALWAYS_INLINE RTS_PURE constexpr operator const_vptr<T,A> () const noexcept {
        return make_const_vptr(v,i);
      }

      RTS_ALWAYS_INLINE vptr & operator = (const vptr & rhs) noexcept = default;
      RTS_ALWAYS_INLINE vptr & operator = (vptr && rhs) noexcept = default;
      RTS_ALWAYS_INLINE vptr & operator += (std::ptrdiff_t j) noexcept;
      RTS_ALWAYS_INLINE vptr & operator -= (std::ptrdiff_t j) noexcept;

      RTS_ALWAYS_INLINE void swap(vptr & that) noexcept {
        std::swap(v,that.v);
        std::swap(i,that.i);
      }
    };

    template <class T, class A>
    RTS_ALWAYS_INLINE RTS_CONST constexpr vptr<T,A> make_vptr(vec<T,A> * v, std::int32_t i = 0) noexcept {
      return vptr<T,A>(v,i);
    }

    template <class T, class A>
    RTS_ALWAYS_INLINE RTS_PURE constexpr vptr<T, A> vref<T,A>::operator & () const noexcept {
      return make_vptr(&v,i);
    }

    // helpers
    template <class T, class A>
    RTS_ALWAYS_INLINE RTS_CONST constexpr const_vref<T,A> make_const_vref(const vec<T,A> & v, std::int32_t i) noexcept {
      return const_vref<T,A>(v,i);
    }
  }

  // default partial template specialization for boolean masks
  template <class A>
  struct vec<bool,A> {
    static_assert(A::width<=32,"vector too large");

    using arch = A;
    using value_type = bool;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using reference = detail::vref<value_type,arch>;
    using const_reference = detail::const_vref<value_type,arch>;
    using pointer = detail::vptr<value_type,arch>;
    using const_pointer = detail::const_vptr<value_type,arch>;
    using iterator = detail::vptr<value_type,arch>;
    using const_iterator = detail::const_vptr<value_type,arch>;

    RTS_ALWAYS_INLINE RTS_CONST constexpr iterator begin() noexcept { return iterator(this); }
    RTS_ALWAYS_INLINE RTS_CONST constexpr iterator end() noexcept { return iterator(this+1); }
    RTS_ALWAYS_INLINE RTS_CONST constexpr const_iterator begin() const noexcept { return const_iterator(this); }
    RTS_ALWAYS_INLINE RTS_CONST constexpr const_iterator end() const noexcept { return const_iterator(this+1); }
    RTS_ALWAYS_INLINE RTS_CONST constexpr const_iterator cbegin() const noexcept { return const_iterator(this); }
    RTS_ALWAYS_INLINE RTS_CONST constexpr const_iterator cend() const noexcept { return const_iterator(this+1); }
    RTS_ALWAYS_INLINE RTS_CONST constexpr reference operator [] (int i) noexcept { return begin()[i]; }
    RTS_ALWAYS_INLINE RTS_CONST constexpr const_reference operator [] (int i) const noexcept { return cbegin()[i]; }

    std::uint32_t data;

    RTS_ALWAYS_INLINE constexpr vec() noexcept : data(0) {}
    RTS_ALWAYS_INLINE constexpr vec(std::uint32_t data, detail::internal_t) noexcept : data(data) {};
    RTS_ALWAYS_INLINE constexpr vec(bool b) noexcept : data (b?arch::width_mask:0) {}
    RTS_ALWAYS_INLINE constexpr vec(const vec & rhs) noexcept = default;
    RTS_ALWAYS_INLINE constexpr vec(std::false_type) noexcept : data(0) {}
    RTS_ALWAYS_INLINE constexpr vec(std::true_type) noexcept : data(arch::width_mask) {}
    RTS_ALWAYS_INLINE vec(vec && rhs) noexcept = default;

    template <typename U>
    RTS_ALWAYS_INLINE constexpr vec(vec<U,arch> & that) noexcept : data(0) {
      for (int i =0;i<arch::width;++i) if (that.get(i)) data |= 1ul << i;
    }

    RTS_ALWAYS_INLINE constexpr vec(std::initializer_list<bool> bs) noexcept : data(0) {
      int i = 1;
      for (auto b : bs) {
        if (b) data |= i;
        i <<= 1;
      }
    }

    RTS_ALWAYS_INLINE RTS_PURE constexpr vec operator ~ () const noexcept { return vec(data ^ A::width_mask, detail::internal_tag); }
    RTS_ALWAYS_INLINE RTS_PURE constexpr vec operator ! () const noexcept { return vec(data ^ A::width_mask, detail::internal_tag); }
    RTS_ALWAYS_INLINE RTS_PURE constexpr vec operator & (const vec & rhs) const noexcept { return vec(data & rhs.data, detail::internal_tag); }
    RTS_ALWAYS_INLINE RTS_PURE constexpr vec operator | (const vec & rhs) const noexcept { return vec(data | rhs.data, detail::internal_tag); }
    RTS_ALWAYS_INLINE RTS_PURE constexpr vec operator ^ (const vec & rhs) const noexcept { return vec(data ^ rhs.data, detail::internal_tag); }
    RTS_ALWAYS_INLINE RTS_PURE constexpr vec operator && (const vec & rhs) const noexcept { return vec(data & rhs.data, detail::internal_tag); }
    RTS_ALWAYS_INLINE RTS_PURE constexpr vec operator || (const vec & rhs) const noexcept { return vec(data | rhs.data, detail::internal_tag); }
    RTS_ALWAYS_INLINE vec & operator = (const vec & rhs) noexcept = default;
    RTS_ALWAYS_INLINE vec & operator = (vec && rhs) noexcept = default;
    RTS_ALWAYS_INLINE vec & operator &= (const vec & rhs) noexcept { data &= rhs.data; return *this; }
    RTS_ALWAYS_INLINE vec & operator |= (const vec & rhs) noexcept { data |= rhs.data; return *this; }
    RTS_ALWAYS_INLINE vec & operator ^= (const vec & rhs) noexcept { data ^= rhs.data; return *this; }
    RTS_ALWAYS_INLINE void swap (vec<bool,A> & that) noexcept { std::swap(data,that.data); }
    RTS_ALWAYS_INLINE RTS_PURE constexpr bool get(int i) const noexcept { return data & (1ul << i); }
    RTS_ALWAYS_INLINE void put(int i, bool b) noexcept { data = b ? data | (1ul << i) : data & ~(1ul << i); }
    RTS_ALWAYS_INLINE RTS_PURE constexpr std::uint32_t movemask() const noexcept { return data; }
  }; // vec<bool,A>

  #ifdef __AVX__
    template <>
    struct vec<bool,target::avx_4> {
      using arch = target::avx_4;
      using value_type = bool;
      using size_type = std::size_t;
      using difference_type = std::ptrdiff_t;
      using reference = detail::vref<value_type,arch>;
      using const_reference = detail::const_vref<value_type,arch>;
      using pointer = detail::vptr<value_type,arch>;
      using const_pointer = detail::const_vptr<value_type,arch>;
      using iterator = detail::vptr<value_type,arch>;
      using const_iterator = detail::const_vptr<value_type,arch>;

      union {
        __m128i m;
        std::int32_t d[4];
      };

      RTS_ALWAYS_INLINE constexpr vec() noexcept : d{0,0,0,0} {}
      RTS_ALWAYS_INLINE constexpr vec(const vec & v) noexcept : m(v.m) {}
      RTS_ALWAYS_INLINE vec(vec && v) noexcept : m(std::move(v.m)) {}
      RTS_ALWAYS_INLINE constexpr vec(std::false_type) noexcept : d{0,0,0,0} {}
      RTS_ALWAYS_INLINE constexpr vec(std::true_type) noexcept : d{~0,~0,~0,~0} {}
      RTS_ALWAYS_INLINE constexpr vec(bool a) : d{a?~0:0,a?~0:0,a?~0:0,a?~0:0} {}
      RTS_ALWAYS_INLINE constexpr vec(__m128i m) noexcept : m(m) {}
      RTS_ALWAYS_INLINE constexpr vec(std::uint32_t m) noexcept : d{m&0x1u?~0:0,m&0x2u?~0:0,m&0x4u?~0:0,m&0x8u?~0:0} {}
      RTS_ALWAYS_INLINE constexpr vec(bool a, bool b, bool c, bool d) noexcept : d{a?~0:0,b?~0:0,c?~0:0,d?~0:0} {}

      template <class U>
      RTS_ALWAYS_INLINE constexpr vec(const vec<U,target::avx_4> & rhs) noexcept
        : vec(rhs.get(0),rhs.get(1),rhs.get(2),rhs.get(3)) { }

      RTS_ALWAYS_INLINE RTS_CONST constexpr iterator begin() noexcept { return iterator(this); }
      RTS_ALWAYS_INLINE RTS_CONST constexpr iterator end() noexcept { return iterator(this+1); }
      RTS_ALWAYS_INLINE RTS_CONST constexpr const_iterator begin() const noexcept { return const_iterator(this); }
      RTS_ALWAYS_INLINE RTS_CONST constexpr const_iterator end() const noexcept { return const_iterator(this+1); }
      RTS_ALWAYS_INLINE RTS_CONST constexpr const_iterator cbegin() const noexcept { return const_iterator(this); }
      RTS_ALWAYS_INLINE RTS_CONST constexpr const_iterator cend() const noexcept { return const_iterator(this+1); }
      RTS_ALWAYS_INLINE RTS_CONST constexpr reference operator [] (int i) noexcept { return begin()[i]; }
      RTS_ALWAYS_INLINE RTS_CONST constexpr const_reference operator [] (int i) const noexcept { return cbegin()[i]; }
      RTS_ALWAYS_INLINE RTS_PURE vec operator ~ () const noexcept { return *this ^ vec(std::true_type()); }
      RTS_ALWAYS_INLINE RTS_PURE vec operator ! () const noexcept { return *this ^ vec(std::true_type()); }
      RTS_ALWAYS_INLINE RTS_PURE vec operator & (const vec & rhs) const noexcept { return _mm_and_si128(m,rhs.m); } // constexpr, portable, and fast. pick 2
      RTS_ALWAYS_INLINE RTS_PURE vec operator | (const vec & rhs) const noexcept { return _mm_or_si128(m,rhs.m); }
      RTS_ALWAYS_INLINE RTS_PURE vec operator ^ (const vec & rhs) const noexcept { return _mm_xor_si128(m,rhs.m); }
      RTS_ALWAYS_INLINE RTS_PURE vec operator && (const vec & rhs) const noexcept { return _mm_and_si128(m,rhs.m); }
      RTS_ALWAYS_INLINE RTS_PURE vec operator || (const vec & rhs) const noexcept { return _mm_or_si128(m,rhs.m); }
      RTS_ALWAYS_INLINE vec & operator = (const vec & rhs) noexcept = default;
      RTS_ALWAYS_INLINE vec & operator = (vec && rhs) noexcept = default;
      RTS_ALWAYS_INLINE vec & operator &= (const vec & rhs) noexcept { m = _mm_and_si128(m,rhs.m); return *this; }
      RTS_ALWAYS_INLINE vec & operator |= (const vec & rhs) noexcept { m = _mm_or_si128(m,rhs.m); return *this; }
      RTS_ALWAYS_INLINE vec & operator ^= (const vec & rhs) noexcept { m = _mm_xor_si128(m,rhs.m); return *this; }
      RTS_ALWAYS_INLINE RTS_PURE bool get(int i) noexcept { return movemask() & (1ul << i); }
      RTS_ALWAYS_INLINE void put(int i, bool b) noexcept { reinterpret_cast<std::int32_t*>(&m)[i] = b?~0:0; }
      RTS_ALWAYS_INLINE RTS_PURE std::uint32_t movemask() const noexcept { return _mm_movemask_ps(_mm_castsi128_ps(m)); }
    }; // vec<bool,avx_4>
  #endif

  #ifdef __AVX2__
    template <>
    struct vec<bool,target::avx2_8> {
      using arch = target::avx2_8;
      using value_type = bool;
      using size_type = std::size_t;
      using difference_type = std::ptrdiff_t;
      using reference = detail::vref<value_type,arch>;
      using const_reference = detail::const_vref<value_type,arch>;
      using pointer = detail::vptr<value_type,arch>;
      using const_pointer = detail::const_vptr<value_type,arch>;
      using iterator = detail::vptr<value_type,arch>;
      using const_iterator = detail::const_vptr<value_type,arch>;

      RTS_ALWAYS_INLINE RTS_CONST constexpr iterator begin() noexcept { return iterator(this); }
      RTS_ALWAYS_INLINE RTS_CONST constexpr iterator end() noexcept { return iterator(this+1); }
      RTS_ALWAYS_INLINE RTS_CONST constexpr const_iterator begin() const noexcept { return const_iterator(this); }
      RTS_ALWAYS_INLINE RTS_CONST constexpr const_iterator end() const noexcept { return const_iterator(this+1); }
      RTS_ALWAYS_INLINE RTS_CONST constexpr const_iterator cbegin() const noexcept { return const_iterator(this); }
      RTS_ALWAYS_INLINE RTS_CONST constexpr const_iterator cend() const noexcept { return const_iterator(this+1); }
      RTS_ALWAYS_INLINE RTS_CONST constexpr reference operator [] (int i) noexcept { return begin()[i]; }
      RTS_ALWAYS_INLINE RTS_CONST constexpr const_reference operator [] (int i) const noexcept { return cbegin()[i]; }

      union {
        __m256i m;
        std::int32_t d[8];
      };

      RTS_ALWAYS_INLINE constexpr vec() noexcept : d{0,0,0,0,0,0,0,0} {}
      RTS_ALWAYS_INLINE constexpr vec(const vec & v) noexcept : m(v.m) {}
      RTS_ALWAYS_INLINE vec(vec && v) noexcept = default;

      template <class U>
      RTS_ALWAYS_INLINE vec(const vec<U,target::avx2_8> & rhs) : vec() {
        for (int i=0;i<8;++i)
          if (rhs.get(i))
            reinterpret_cast<std::int32_t*>(&m)[i] = ~0;
      }

      RTS_ALWAYS_INLINE constexpr vec(std::false_type) noexcept : d{0,0,0,0,0,0,0,0} {}
      RTS_ALWAYS_INLINE constexpr vec(std::true_type) noexcept : d{~0,~0,~0,~0,~0,~0,~0,~0} {}
      RTS_ALWAYS_INLINE constexpr vec(bool b) noexcept :  d{b?~0:0,b?~0:0,b?~0:0,b?~0:0,b?~0:0,b?~0:0,b?~0:0,b?~0:0} {}
      RTS_ALWAYS_INLINE constexpr vec(__m256i m, detail::internal_t) noexcept : m(m) {}
      RTS_ALWAYS_INLINE constexpr vec(std::uint32_t m) noexcept : d{m&0x01u?~0:0,m&0x02u?~0:0,m&0x04u?~0:0,m&0x08u?~0:0,m&0x10u?~0:0,m&0x20u?~0:0,m&0x40u?~0:0,m&0x80u?~0:0} {}
      RTS_ALWAYS_INLINE constexpr vec(bool a, bool b, bool c, bool d, bool e, bool f, bool g, bool h) noexcept : d{a?~0:0,b?~0:0,c?~0:0,d?~0:0,e?~0:0,f?~0:0,g?~0:0,h?~0:0} {}

      RTS_ALWAYS_INLINE RTS_PURE vec operator ~ () noexcept { return *this ^ vec(std::true_type()); }
      RTS_ALWAYS_INLINE RTS_PURE vec operator ! () noexcept { return *this ^ vec(std::true_type()); }
      RTS_ALWAYS_INLINE RTS_PURE vec operator & (const vec & rhs) const noexcept { return vec(_mm256_and_si256(m,rhs.m), detail::internal_tag); }
      RTS_ALWAYS_INLINE RTS_PURE vec operator | (const vec & rhs) const noexcept { return vec(_mm256_or_si256(m,rhs.m), detail::internal_tag); }
      RTS_ALWAYS_INLINE RTS_PURE vec operator ^ (const vec & rhs) const noexcept { return vec(_mm256_xor_si256(m,rhs.m), detail::internal_tag); }
      RTS_ALWAYS_INLINE RTS_PURE vec operator && (const vec & rhs) const noexcept { return vec(_mm256_and_si256(m,rhs.m), detail::internal_tag); }
      RTS_ALWAYS_INLINE RTS_PURE vec operator || (const vec & rhs) const noexcept { return vec(_mm256_or_si256(m,rhs.m), detail::internal_tag); }
      RTS_ALWAYS_INLINE vec & operator = (const vec & rhs) noexcept = default;
      RTS_ALWAYS_INLINE vec & operator = (vec && rhs) noexcept = default;
      RTS_ALWAYS_INLINE vec & operator &= (const vec & rhs) noexcept { m = _mm256_and_si256(m,rhs.m); return *this; }
      RTS_ALWAYS_INLINE vec & operator |= (const vec & rhs) noexcept { m = _mm256_or_si256(m,rhs.m); return *this; }
      RTS_ALWAYS_INLINE vec & operator ^= (const vec & rhs) noexcept { m = _mm256_xor_si256(m,rhs.m); return *this; }
      RTS_ALWAYS_INLINE RTS_PURE bool get(int i) noexcept { return movemask()&(1<<i); }
      RTS_ALWAYS_INLINE void put(int i, bool b) noexcept { reinterpret_cast<std::int32_t*>(&m)[i] = b?~0:0; }
      RTS_ALWAYS_INLINE RTS_PURE std::uint32_t movemask() const noexcept { return _mm256_movemask_ps(_mm256_castsi256_ps(m)); }
    }; // vec<bool,avx2_8>
  #endif

  template <class A>
  RTS_ALWAYS_INLINE RTS_PURE std::uint32_t movemask(const vec<bool,A> & m) noexcept {
    return m.movemask();
  }

  template <class A>
  RTS_ALWAYS_INLINE RTS_PURE std::uint32_t any(const vec<bool,A> & m) noexcept {
    return movemask(m) != 0;
  }

  template <class A>
  RTS_ALWAYS_INLINE RTS_PURE std::uint32_t all(const vec<bool,A> & m) noexcept {
    return movemask(m) == A::width_mask;
  }

  template <class A, class F>
  RTS_ALWAYS_INLINE void foreach_active(const vec<bool,A> & mask, F f) {
    std::uint32_t m = movemask(mask);
    while (m) {
      int i = bscf(m); // mutates m
      f(i);
    }
  }

  // generic fallback. can load from things like std::vector<bool> and other random access containers.
  template <class T, class U, class A>
  RTS_ALWAYS_INLINE void load(vec<T,A> & v, U mem, const vec<bool,A> & mask) {
    foreach_active(mask, [&](int i) { v.put(i,mem[i]); });
  }

  namespace detail {
    template <class T, class A> struct loader;

    template <class T, class A>
    struct base_loader {
      using element_type = T;
      using arch = A;
      using vector = vec<element_type,arch>;
      using pointers = const vec<element_type*,arch> &;
      using mask = const vec<bool,arch> &;

      RTS_ALWAYS_INLINE static void load_masked(vector & u, pointers v, mask m) {
        foreach_active(m, [&](int i) { u.put(i, *v.get(i)); });
      }

      RTS_ALWAYS_INLINE static void load(vector & u, pointers v) {
        for (int i=0;i<arch::width;++i) u.put(i,*v.get(i));
      }

      RTS_ALWAYS_INLINE static void store_masked(pointers v, const vector & u, mask m) {
        foreach_active(m, [&](int i) { *v.get(i) = u.get(i); });
      }

      RTS_ALWAYS_INLINE static void store(pointers v, const vector & u) {
        for (int i=0;i<arch::width;++i) *v.get(i) = u.get(i);
      }
    };

    // default loader
    template <class T, class A> struct loader : base_loader<T,A> {};
  }

  #ifdef __AVX__
    template <>
    struct vec<std::int32_t, target::avx_4> {
      using arch = target::avx_4;
      using size_type = std::size_t;
      using difference_type = std::ptrdiff_t;
      using value_type = std::int32_t;
      using reference = value_type &;
      using const_reference = const value_type &;
      using pointer = value_type *;
      using const_pointer = const value_type *;
      using iterator = pointer;
      using const_iterator = const value_type *;

      union {
        __m128i m;
        std::int32_t d[4];
      };

      RTS_ALWAYS_INLINE constexpr vec() noexcept : d{0,0,0,0} {}
      RTS_ALWAYS_INLINE constexpr vec(std::int32_t i) noexcept : d{i,i,i,i} {}
      RTS_ALWAYS_INLINE constexpr vec(const vec & rhs) noexcept : m(rhs.m) {}
      RTS_ALWAYS_INLINE constexpr vec(std::int32_t a, std::int32_t b, std::int32_t c, std::int32_t d) noexcept : d{a,b,c,d} {}
      RTS_ALWAYS_INLINE constexpr vec(__m128i m) noexcept : m(m) {}
      RTS_ALWAYS_INLINE vec(vec && rhs) noexcept : m(std::move(rhs.m)) {}

      RTS_ALWAYS_INLINE RTS_CONST constexpr iterator begin() noexcept { return d; }
      RTS_ALWAYS_INLINE RTS_CONST constexpr iterator end() noexcept { return d + arch::width; }
      RTS_ALWAYS_INLINE RTS_CONST constexpr const_iterator begin() const noexcept { return d; }
      RTS_ALWAYS_INLINE RTS_CONST constexpr const_iterator end() const noexcept { return d + arch::width; }
      RTS_ALWAYS_INLINE RTS_CONST constexpr const_iterator cbegin() const noexcept { return d; }
      RTS_ALWAYS_INLINE RTS_CONST constexpr const_iterator cend() const noexcept { return d + arch::width; }
      RTS_ALWAYS_INLINE RTS_CONST constexpr reference operator [] (int i) noexcept { return d[i]; }
      RTS_ALWAYS_INLINE RTS_CONST constexpr const_reference operator [] (int i) const noexcept { return d[i]; }
      RTS_ALWAYS_INLINE RTS_CONST constexpr reference get(int i) noexcept { return d[i]; }
      RTS_ALWAYS_INLINE RTS_CONST constexpr const_reference get(int i) const noexcept { return d[i]; }
      RTS_ALWAYS_INLINE void put (int i, std::int32_t v) noexcept { d[i] = v; }

      RTS_ALWAYS_INLINE vec & operator = (const vec & rhs) noexcept { m = rhs.m; return *this; }
      RTS_ALWAYS_INLINE vec & operator ^= (const vec & rhs) noexcept { m = _mm_xor_si128(m,rhs.m); return *this; }
      RTS_ALWAYS_INLINE vec & operator += (const vec & rhs) noexcept { m = _mm_add_epi32(m,rhs.m); return *this; }
      RTS_ALWAYS_INLINE vec & operator *= (const vec & rhs) noexcept { m = _mm_mullo_epi32(m,rhs.m); return *this; }

      RTS_ALWAYS_INLINE vec & operator ++ () noexcept {
        for (auto && r : d) ++r;
        return *this;
      }

      RTS_ALWAYS_INLINE vec operator ++ (int) noexcept {
        vec t(*this);
        operator++();
        return t;
      }

      RTS_ALWAYS_INLINE vec & operator -- () noexcept {
        for (auto && r : d) --r;
        return *this;
      }

      RTS_ALWAYS_INLINE vec operator -- (int) noexcept {
        vec t(*this);
        operator--();
        return t;
      }
    };
  #endif

  #ifdef __AVX2__
    template <>
    struct vec<std::int32_t, target::avx2_8> {
      using arch = target::avx2_8;
      using size_type = std::size_t;
      using difference_type = std::ptrdiff_t;
      using value_type = std::int32_t;
      using reference = value_type &;
      using const_reference = const value_type &;
      using pointer = value_type *;
      using const_pointer = const value_type *;
      using iterator = pointer;
      using const_iterator = const value_type *;

      union {
        __m256i m;
        struct { __m128i l,h; };
        std::int32_t d[8];
      };

      RTS_ALWAYS_INLINE constexpr vec() noexcept : d{0,0,0,0,0,0,0,0} {}
      RTS_ALWAYS_INLINE constexpr vec(std::int32_t i) noexcept : d{i,i,i,i,i,i,i,i} {}
      RTS_ALWAYS_INLINE constexpr vec(const vec & rhs) noexcept : m(rhs.m) {}
      RTS_ALWAYS_INLINE constexpr vec(std::int32_t a, std::int32_t b, std::int32_t c, std::int32_t d,
          std::int32_t e, std::int32_t f, std::int32_t g, std::int32_t h) noexcept : d{a,b,c,d,e,f,g,h} {}
      RTS_ALWAYS_INLINE vec(vec && rhs) noexcept : m(std::move(rhs.m)) {}

      RTS_ALWAYS_INLINE RTS_CONST constexpr iterator begin() noexcept { return d; }
      RTS_ALWAYS_INLINE RTS_CONST constexpr iterator end() noexcept { return d + arch::width; }
      RTS_ALWAYS_INLINE RTS_CONST constexpr const_iterator begin() const noexcept { return d; }
      RTS_ALWAYS_INLINE RTS_CONST constexpr const_iterator end() const noexcept { return d + arch::width; }
      RTS_ALWAYS_INLINE RTS_CONST constexpr const_iterator cbegin() const noexcept { return d; }
      RTS_ALWAYS_INLINE RTS_CONST constexpr const_iterator cend() const noexcept { return d + arch::width; }
      RTS_ALWAYS_INLINE RTS_CONST constexpr reference operator [] (int i) noexcept { return begin()[i]; }
      RTS_ALWAYS_INLINE RTS_CONST constexpr const_reference operator [] (int i) const noexcept { return cbegin()[i]; }
      RTS_ALWAYS_INLINE RTS_CONST constexpr reference get(int i) noexcept { return d[i]; }
      RTS_ALWAYS_INLINE RTS_CONST constexpr const_reference get(int i) const noexcept { return d[i]; }
      RTS_ALWAYS_INLINE void put (int i, std::int32_t v) noexcept { d[i] = v; }

      RTS_ALWAYS_INLINE vec & operator = (const vec & rhs) noexcept { m = rhs.m; return *this; }
      RTS_ALWAYS_INLINE vec & operator ^= (const vec & rhs) noexcept { m = _mm256_xor_si256(m,rhs.m); return *this; }
      RTS_ALWAYS_INLINE vec & operator += (const vec & rhs) noexcept { m = _mm256_add_epi32(m,rhs.m); return *this; }
      RTS_ALWAYS_INLINE vec & operator *= (const vec & rhs) noexcept { m = _mm256_mullo_epi32(m,rhs.m); return *this; }

      RTS_ALWAYS_INLINE vec & operator ++ () noexcept {
        for (auto && r : d) ++r;
        return *this;
      }

      RTS_ALWAYS_INLINE vec operator ++ (int) noexcept {
        vec t(*this);
        operator++();
        return t;
      }

      RTS_ALWAYS_INLINE vec & operator -- () noexcept {
        for (auto && r : d) --r;
        return *this;
      }

      RTS_ALWAYS_INLINE vec operator -- (int) noexcept {
        vec t(*this);
        operator--();
        return t;
      }
    };
  #endif

  #ifdef __AVX__
    template <>
    struct vec<float, target::avx_4> {
      using arch = target::avx_4;
      using value_type = float;
      using size_type = std::size_t;
      using difference_type = std::ptrdiff_t;
      using reference = value_type &;
      using const_reference = const value_type &;
      using pointer = value_type *;
      using const_pointer = const value_type *;
      using iterator = pointer;
      using const_iterator = const value_type *;

      union {
        __m128 m;
        float d[4];
      };

      RTS_ALWAYS_INLINE constexpr vec() noexcept : d{0.f,0.f,0.f,0.f} {}
      RTS_ALWAYS_INLINE constexpr vec(float f) noexcept : d{f,f,f,f} {}
      RTS_ALWAYS_INLINE constexpr vec(const vec & rhs) noexcept : m(rhs.m) {}
      RTS_ALWAYS_INLINE constexpr vec(float a, float b, float c, float d) noexcept : d{a,b,c,d} {}
      RTS_ALWAYS_INLINE vec(vec && rhs) : m(std::move(rhs.m)) {}

      RTS_ALWAYS_INLINE RTS_CONST constexpr iterator begin() noexcept { return d; }
      RTS_ALWAYS_INLINE RTS_CONST constexpr iterator end() noexcept { return d + arch::width; }
      RTS_ALWAYS_INLINE RTS_CONST constexpr const_iterator begin() const noexcept { return d; }
      RTS_ALWAYS_INLINE RTS_CONST constexpr const_iterator end() const noexcept { return d + arch::width; }
      RTS_ALWAYS_INLINE RTS_CONST constexpr const_iterator cbegin() const noexcept { return d; }
      RTS_ALWAYS_INLINE RTS_CONST constexpr const_iterator cend() const noexcept { return d + arch::width; }
      RTS_ALWAYS_INLINE RTS_CONST constexpr reference operator [] (int i) noexcept { return d[i]; }
      RTS_ALWAYS_INLINE RTS_CONST constexpr const_reference operator [] (int i) const noexcept { return d[i]; }
      RTS_ALWAYS_INLINE RTS_CONST constexpr reference get(int i) noexcept { return d[i]; }
      RTS_ALWAYS_INLINE RTS_CONST constexpr const_reference get(int i) const noexcept { return d[i]; }
      RTS_ALWAYS_INLINE void put (int i, float v) noexcept { d[i] = v; }
      RTS_ALWAYS_INLINE vec & operator = (const vec & rhs) noexcept { m = rhs.m; return *this; }
      RTS_ALWAYS_INLINE vec & operator += (const vec & rhs) noexcept { m = _mm_add_ps(m,rhs.m); return *this; }
      RTS_ALWAYS_INLINE vec & operator -= (const vec & rhs) noexcept { m = _mm_sub_ps(m,rhs.m); return *this; }
      RTS_ALWAYS_INLINE vec & operator *= (const vec & rhs) noexcept { m = _mm_mul_ps(m,rhs.m); return *this; }
    };
  #endif

  #ifdef __AVX2__
    template <>
    struct vec<float, target::avx2_8> {
      using arch = target::avx2_8;
      using value_type = float;
      using size_type = std::size_t;
      using difference_type = std::ptrdiff_t;
      using reference = value_type &;
      using const_reference = const value_type &;
      using pointer = value_type *;
      using const_pointer = const value_type *;
      using iterator = pointer;
      using const_iterator = const value_type *;

      union {
        __m256 m;
        struct { __m128 l,h; };
        float d[8];
      };

      RTS_ALWAYS_INLINE constexpr vec() noexcept : d{0.f,0.f,0.f,0.f,0.f,0.f,0.f,0.f} {}
      RTS_ALWAYS_INLINE constexpr vec(float f) noexcept : d{f,f,f,f,f,f,f,f} {}
      RTS_ALWAYS_INLINE constexpr vec(const vec & rhs) noexcept : m(rhs.m) {}
      RTS_ALWAYS_INLINE constexpr vec(float a, float b, float c, float d, float e, float f, float g, float h) noexcept : d{a,b,c,d,e,f,g,h} {}
      RTS_ALWAYS_INLINE vec(vec && rhs) noexcept : m(std::move(rhs.m)) {}

      RTS_ALWAYS_INLINE RTS_CONST constexpr iterator begin() noexcept { return d; }
      RTS_ALWAYS_INLINE RTS_CONST constexpr iterator end() noexcept { return d + arch::width; }
      RTS_ALWAYS_INLINE RTS_CONST constexpr const_iterator begin() const noexcept { return d; }
      RTS_ALWAYS_INLINE RTS_CONST constexpr const_iterator end() const noexcept { return d + arch::width; }
      RTS_ALWAYS_INLINE RTS_CONST constexpr const_iterator cbegin() const noexcept { return d; }
      RTS_ALWAYS_INLINE RTS_CONST constexpr const_iterator cend() const noexcept { return d + arch::width; }
      RTS_ALWAYS_INLINE RTS_CONST constexpr reference operator [] (int i) noexcept { return d[i]; }
      RTS_ALWAYS_INLINE RTS_CONST constexpr const_reference operator [] (int i) const noexcept { return d[i]; }
      RTS_ALWAYS_INLINE RTS_CONST constexpr reference get(int i) noexcept { return d[i]; }
      RTS_ALWAYS_INLINE RTS_CONST constexpr const_reference get(int i) const noexcept { return d[i]; }
      RTS_ALWAYS_INLINE void put (int i, float v) noexcept { d[i] = v; }
      RTS_ALWAYS_INLINE vec & operator = (const vec & rhs) noexcept { m = rhs.m; return *this; }
      RTS_ALWAYS_INLINE vec & operator += (const vec & rhs) noexcept { m = _mm256_add_ps(m,rhs.m); return *this; }
      RTS_ALWAYS_INLINE vec & operator -= (const vec & rhs) noexcept { m = _mm256_sub_ps(m,rhs.m); return *this; }
      RTS_ALWAYS_INLINE vec & operator *= (const vec & rhs) noexcept { m = _mm256_mul_ps(m,rhs.m); return *this; }
    };
  #endif

  #ifdef __AVX__
    // we need vec<T * const,avx2_8> as well
    template <class T>
    struct vec<T*,target::avx_4> {
      using arch = target::avx_4;
      using value_type = T*;
      using reference = T*&;
      using const_reference = T* const &;
      using pointer = T**;
      using const_pointer = const T**;
      using iterator = pointer;
      using const_iterator = const T**;

      union {
        #ifdef RTS_64
          __m128i m[2]; // we need two registers
        #else
          __m128i m;
        #endif
        T * d[4];
      };

      RTS_ALWAYS_INLINE constexpr vec() noexcept : d{nullptr,nullptr,nullptr,nullptr} {}
      RTS_ALWAYS_INLINE constexpr vec(std::nullptr_t) noexcept : d{nullptr,nullptr,nullptr,nullptr} {}
      RTS_ALWAYS_INLINE constexpr vec(T * a) noexcept : d(a,a,a,a) {}
      RTS_ALWAYS_INLINE constexpr vec(const vec & rhs) noexcept : m(d.m) {}
      RTS_ALWAYS_INLINE vec(vec && rhs) noexcept : m(std::move(d.m)) {}

      RTS_ALWAYS_INLINE RTS_CONST constexpr iterator begin() noexcept { return d; }
      RTS_ALWAYS_INLINE RTS_CONST constexpr iterator end() noexcept { return d + arch::width; }
      RTS_ALWAYS_INLINE RTS_CONST constexpr const_iterator begin() const noexcept { return d; }
      RTS_ALWAYS_INLINE RTS_CONST constexpr const_iterator end() const noexcept { return d + arch::width; }
      RTS_ALWAYS_INLINE RTS_CONST constexpr const_iterator cbegin() const noexcept { return d; }
      RTS_ALWAYS_INLINE RTS_CONST constexpr const_iterator cend() const noexcept { return d + arch::width; }
      RTS_ALWAYS_INLINE RTS_CONST constexpr reference operator [] (int i) noexcept { return d[i]; }
      RTS_ALWAYS_INLINE RTS_CONST constexpr const_reference operator [] (int i) const noexcept { return d[i]; }
      RTS_ALWAYS_INLINE RTS_CONST constexpr reference get(int i) noexcept { return d[i]; }
      RTS_ALWAYS_INLINE RTS_CONST constexpr const_reference get(int i) const noexcept { return d[i]; }
      RTS_ALWAYS_INLINE void put(int i, T * p) noexcept { d[i] = p; }
      RTS_ALWAYS_INLINE vec & operator = (const vec & rhs) noexcept { m = rhs.m; return *this; }
    };

    // template <> struct loader<float,avx_4> : base_loader<float,avx_4> {}

  #endif

  #ifdef __AVX2__
    // we need vec<T * const,avx2_8> as well
    template <class T>
    struct vec<T*,target::avx2_8> {
      using arch = target::avx2_8;
      using value_type = T*;
      using reference = T*&;
      using const_reference = T* const &;
      using pointer = T**;
      using const_pointer = const T**;
      using iterator = pointer;
      using const_iterator = const T**;

      union {
        #ifdef RTS_64
          __m256i m[2]; // we need two registers
        #else
          __m256i m;
        #endif
        T * d[8];
      };

      RTS_ALWAYS_INLINE constexpr vec() noexcept : d{nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,nullptr} {}
      RTS_ALWAYS_INLINE constexpr vec(std::nullptr_t) noexcept : d{nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,nullptr} {}
      RTS_ALWAYS_INLINE constexpr vec(T * a) noexcept : d(a,a,a,a,a,a,a,a) {}
      RTS_ALWAYS_INLINE constexpr vec(const vec & rhs) noexcept : m(d.m) {}
      RTS_ALWAYS_INLINE vec(vec && rhs) noexcept : m(std::move(d.m)) {}

      RTS_ALWAYS_INLINE RTS_CONST constexpr iterator begin() noexcept { return d; }
      RTS_ALWAYS_INLINE RTS_CONST constexpr iterator end() noexcept { return d + arch::width; }
      RTS_ALWAYS_INLINE RTS_CONST constexpr const_iterator begin() const noexcept { return d; }
      RTS_ALWAYS_INLINE RTS_CONST constexpr const_iterator end() const noexcept { return d + arch::width; }
      RTS_ALWAYS_INLINE RTS_CONST constexpr const_iterator cbegin() const noexcept { return d; }
      RTS_ALWAYS_INLINE RTS_CONST constexpr const_iterator cend() const noexcept { return d + arch::width; }
      RTS_ALWAYS_INLINE RTS_CONST constexpr reference operator [] (int i) noexcept { return d[i]; }
      RTS_ALWAYS_INLINE RTS_CONST constexpr const_reference operator [] (int i) const noexcept { return d[i]; }
      RTS_ALWAYS_INLINE RTS_CONST constexpr reference get(int i) noexcept { return d[i]; }
      RTS_ALWAYS_INLINE RTS_CONST constexpr const_reference get(int i) const noexcept { return d[i]; }
      RTS_ALWAYS_INLINE void put(int i, T * p) noexcept { begin()[i] = p; }
      RTS_ALWAYS_INLINE vec & operator = (const vec & rhs) noexcept { m = rhs.m; return *this; }
    };

    namespace detail {

      // this should probably be disabled as manual gathers are faster than avx2 gathers
      // but this allows us to test the code path
      template <>
      struct loader<std::int32_t,target::avx2_8> : base_loader<std::int32_t,target::avx2_8> {
        RTS_ALWAYS_INLINE static void load_masked(vector & u, pointers v, mask m) noexcept {
          #ifdef RTS_64
            // i'd merge these with _mm256_set_m128i, but gcc helpfully forgot to include it
            u.h = _mm256_mask_i64gather_epi32(
                _mm256_extracti128_si256(u.m,1)
              , nullptr
              , v.m[1]
              , _mm256_extracti128_si256(m.m,1) // upper half of mask
              , 1);
            u.l = _mm256_mask_i64gather_epi32(
                _mm256_extracti128_si256(u.m,0)
              , nullptr // base
              , v.m[0]
              , _mm256_extracti128_si256(m.m,0) // low half of mask
              , 1);
          #else
            u.m = _mm256_mask_i32gather_epi32(u.m, nullptr, v.m, m.m, 1);
          #endif
        }

        RTS_ALWAYS_INLINE static void load(vector & u, pointers v) noexcept {
          #if RTS_64
            u.h = _mm256_i64gather_epi32(nullptr, v.m[1], 1);
            u.l = _mm256_i64gather_epi32(nullptr, v.m[0], 1);
          #else
            u.m = _mm256_i32gather_epi32(nullptr, v.m, 1);
          #endif
        }
      };

      template <>
      struct loader<float,target::avx2_8> : base_loader<float,target::avx2_8> {
        RTS_ALWAYS_INLINE static void load_masked(vector & u, pointers v, mask m) noexcept {
          #ifdef RTS_64
            u.l = _mm256_mask_i64gather_ps(
                  _mm256_extractf128_ps(u.m,0)
                , nullptr
                , v.m[0]
                , _mm_castsi128_ps(_mm256_extracti128_si256(m.m,0))
                , 1
                );
            u.h = _mm256_mask_i64gather_ps(
                  _mm256_extractf128_ps(u.m,1)
                , nullptr
                , v.m[1]
                , _mm_castsi128_ps(_mm256_extracti128_si256(m.m,1))
                , 1
                );
          #else
            u.m = _mm256_mask_i32gather_ps(u.m, nullptr, v.m, _mm256_castsi256_ps(m.m), 1);
          #endif
        }

        RTS_ALWAYS_INLINE static void load(vector & u, pointers v) noexcept {
          #ifdef RTS_64
            u.l = _mm256_i64gather_ps(nullptr, v.m[0], 1);
            u.h = _mm256_i64gather_ps(nullptr, v.m[1], 1);
          #else
            u.m = _mm256_i32gather_ps(nullptr, v.m, 1);
          #endif
        }
      };
    }

  #endif // AVX2

  template <class T, class A>
  RTS_ALWAYS_INLINE vec<T,A> load(const vec<T*,A> & pointers, const vec<bool,A> & mask) noexcept {
    vec<T,A> result;
    detail::loader<T,A>::load(result, pointers, mask);
    return result;
  }

  template <class T, class A>
  RTS_ALWAYS_INLINE vec<T,A> load(const vec<T*,A> & pointers) noexcept {
    vec<T,A> result;
    detail::loader<T,A>::load(result, pointers);
    return result;
  }

  template <class T, class A>
  RTS_ALWAYS_INLINE void store(const vec<T*,A> & pointers, const vec<T, A> & t, const vec<bool,A> & mask) noexcept {
    detail::loader<T,A>::store(pointers, t, mask);
  }

  template <class T, class A>
  RTS_ALWAYS_INLINE void store(const vec<T*,A> & pointers, const vec<T, A> & t) noexcept {
    detail::loader<T,A>::store(pointers, t);
  }

  namespace detail {
    template <class T, class A> struct vrefptr;

    // reference to a reference
    template <class T, class A>
    struct vrefref {
      typename vec<T*,A>::const_reference r;
      vrefref() noexcept = delete;
      constexpr vrefref(const vrefref &) noexcept = default;
      vrefref(vrefref && rhs) noexcept = default;
      RTS_ALWAYS_INLINE explicit constexpr vrefref(const typename vec<T*,A>::const_reference & r) noexcept : r(r) {}
      RTS_ALWAYS_INLINE RTS_CONST constexpr T & get() const noexcept { return *r.get(); }
      RTS_ALWAYS_INLINE RTS_CONST constexpr operator T & () const noexcept { return *r.get(); }
      constexpr vrefptr<T,A> operator & () const noexcept;
    };

    template <class T, class A>
    struct vrefptr {
      // faux "pointer to a reference"
      typename vec<T*,A>::const_pointer p;
      RTS_ALWAYS_INLINE constexpr vrefptr () noexcept : p() {}
      RTS_ALWAYS_INLINE explicit constexpr vrefptr(const typename vec<T*,A>::const_pointer & p) noexcept : p(p) {}
      RTS_ALWAYS_INLINE vrefptr(vrefptr && rhs) noexcept = default;
      RTS_ALWAYS_INLINE RTS_PURE constexpr vrefref<T,A> operator * () noexcept { return vrefref<T,A>(*p); }
      // we need operators to manipulate these
      RTS_ALWAYS_INLINE RTS_PURE constexpr vrefptr operator - (std::ptrdiff_t rhs) const noexcept { return vrefptr(p - rhs); }
      RTS_ALWAYS_INLINE RTS_PURE constexpr vrefptr operator + (std::ptrdiff_t rhs) const noexcept { return vrefptr(p + rhs); }
      RTS_ALWAYS_INLINE vrefptr & operator ++ () noexcept { ++p; return *this; }
      RTS_ALWAYS_INLINE vrefptr & operator -- () noexcept { --p; return *this; }
      RTS_ALWAYS_INLINE vrefptr operator ++ (int) noexcept { return vrefptr(p++); }
      RTS_ALWAYS_INLINE vrefptr operator -- (int) noexcept { return vrefptr(p--); }
      RTS_ALWAYS_INLINE vrefptr & operator = (const vrefptr & rhs) noexcept { p = rhs.p; return *this; }
      RTS_ALWAYS_INLINE vrefptr & operator += (std::ptrdiff_t d) noexcept { p += d; return *this; }
      RTS_ALWAYS_INLINE vrefptr & operator -= (std::ptrdiff_t d) noexcept { p -= d; return *this; }
      RTS_ALWAYS_INLINE constexpr vrefref<T,A> operator[](std::ptrdiff_t i) const noexcept { return vrefref<T,A>(p[i]); }
    };

    #define RTS_CMP(op) \
      template <class T, class A> \
      RTS_ALWAYS_INLINE RTS_PURE constexpr bool operator op (const vrefptr<T,A> & lhs, const vrefptr<T,A> & rhs) noexcept { \
        return lhs.p op rhs.p; \
      } \
      template <class T, class A> \
      RTS_ALWAYS_INLINE RTS_PURE constexpr bool operator op (const vrefptr<T,A> & lhs, std::nullptr_t) noexcept { \
        return lhs.p op nullptr; \
      } \
      template <class T, class A> \
      RTS_ALWAYS_INLINE RTS_PURE constexpr bool operator op (std::nullptr_t, const vrefptr<T,A> & rhs) noexcept { \
        return nullptr op rhs.p; \
      }

    RTS_CMP(==)
    RTS_CMP(!=)
    RTS_CMP(<=)
    RTS_CMP(<)
    RTS_CMP(>)
    RTS_CMP(>=)

    #undef RTS_CMP

    template <class T, class A>
    RTS_ALWAYS_INLINE RTS_PURE constexpr detail::vrefptr<T,A> detail::vrefref<T,A>::operator & () const noexcept {
      return vrefptr<T,A>(&r);
    }
    // TODO: add left ptrdiff addition, and nullptr comparisons to vec<T,A>::pointer
  }

  // overloaded reference type for references, as endorsed by the department of redundancy department
  template <class T, class A>
  struct vec<T&,A> {
    using arch = A;
    using value_type = T &;
    using base = vec<T*,A>;
    using reference = detail::vrefref<T,A>;
    using pointer = detail::vrefptr<T,A>;
    using iterator = pointer;
    using const_reference = reference;
    using const_pointer = pointer;
    using const_iterator = pointer;

    base pointers;

    vec() = delete;
    vec(const vec & that) = delete;
    vec(vec && that) = delete;
    RTS_ALWAYS_INLINE explicit vec(const vec<T*,A> & that, detail::indirection_t) noexcept : pointers(that) {};
    RTS_ALWAYS_INLINE explicit vec(vec<T*,A> && that, detail::indirection_t) noexcept : pointers(std::move(that)) {}

    RTS_ALWAYS_INLINE RTS_CONST constexpr iterator begin() noexcept { return iterator(pointers.begin()); }
    RTS_ALWAYS_INLINE RTS_CONST constexpr iterator end() noexcept { return iterator(pointers.end()); }
    RTS_ALWAYS_INLINE RTS_CONST constexpr const_iterator begin() const noexcept { return iterator(pointers.cbegin()); }
    RTS_ALWAYS_INLINE RTS_CONST constexpr const_iterator end() const noexcept { return iterator(pointers.cend()); }
    RTS_ALWAYS_INLINE RTS_CONST constexpr const_iterator cbegin() const noexcept { return iterator(pointers.cbegin()); }
    RTS_ALWAYS_INLINE RTS_CONST constexpr const_iterator cend() const noexcept { return iterator(pointers.cend()); }
    RTS_ALWAYS_INLINE RTS_CONST constexpr reference operator [] (int i) noexcept { return reference(begin()[i]); }
    RTS_ALWAYS_INLINE RTS_CONST constexpr const_reference operator [] (int i) const noexcept { return reference(cbegin()[i]); }

    RTS_ALWAYS_INLINE RTS_CONST constexpr T & get(int i) noexcept { return *pointers[i]; }
    RTS_ALWAYS_INLINE RTS_CONST constexpr const T & get(int i) const noexcept { return *pointers[i]; }
    RTS_ALWAYS_INLINE void put(int i, const T & rhs) { *pointers[i] = rhs; }

    RTS_ALWAYS_INLINE vec & operator = (const vec<T,A> & t) const noexcept {
      store(pointers, t);
      return *this;
    }
    
    template <typename U> RTS_ALWAYS_INLINE vec & operator += (U t) const noexcept {
      store(pointers, load(pointers) + t);
      return *this;
    }
    
    template <typename U> RTS_ALWAYS_INLINE vec & operator -= (U t) const noexcept {
      store(pointers, load(pointers) - t);
      return *this;
    }
    
    template <typename U> RTS_ALWAYS_INLINE vec & operator *= (U t) const noexcept {
      store(pointers, load(pointers) * t);
      return *this;
    }
    
    template <typename U> RTS_ALWAYS_INLINE vec & operator /= (U t) const noexcept {
      store(pointers, load(pointers) / t);
      return *this;
    }
    
    template <typename U> RTS_ALWAYS_INLINE vec & operator &= (U t) const noexcept {
      store(pointers, load(pointers) & t);
      return *this;
    }
    
    template <typename U> RTS_ALWAYS_INLINE vec & operator |= (U t) const noexcept {
      store(pointers, load(pointers) | t);
      return *this;
    }
    
    template <typename U> RTS_ALWAYS_INLINE vec & operator <<= (U t) const noexcept {
      store(pointers, load(pointers) << t);
      return *this;
    }

    template <typename U> RTS_ALWAYS_INLINE vec & operator >>= (U t) const noexcept {
      store(pointers, load(pointers) >> t);
      return *this;
    }

    RTS_ALWAYS_INLINE RTS_CONST constexpr vec<T*,A> & operator & () noexcept { return pointers; }
    RTS_ALWAYS_INLINE RTS_CONST constexpr const vec<T*,A> & operator & () const noexcept { return pointers; }
    RTS_ALWAYS_INLINE operator vec<T,A> () { return load(pointers); }
  }; // vec<T&,A>


  namespace detail {
    template <class F, std::size_t... Is>
    RTS_ALWAYS_INLINE constexpr auto index_apply_impl(F f, std::index_sequence<Is...>) {
      return f(std::integral_constant<std::size_t, Is> {}...);
    }

    template <std::size_t N, class F>
    RTS_ALWAYS_INLINE constexpr auto index_apply(F f) {
      return index_apply_impl(f, std::make_index_sequence<N>{});
    }

    template <std::size_t I, typename T, typename V> 
    RTS_ALWAYS_INLINE void put1(T & t, int i, const V & v) {
      std::get<I>(t).put(i,std::get<I>(v));
    }
  }

  template <class A, class ... Ts>
  struct vec<std::tuple<Ts...>, A> {
    using arch = A;
    using value_type = std::tuple<Ts...>;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using reference = detail::vref<value_type,arch>;
    using const_reference = detail::const_vref<value_type,arch>;
    using pointer = detail::vptr<value_type,arch>;
    using const_pointer = detail::const_vptr<value_type,arch>;
    using iterator = detail::vptr<value_type,arch>;
    using const_iterator = detail::const_vptr<value_type,arch>;

    std::tuple<vec<Ts,A>...> data;

    RTS_ALWAYS_INLINE constexpr vec() noexcept = default;
    RTS_ALWAYS_INLINE constexpr vec(const vec & rhs) noexcept = default;
    RTS_ALWAYS_INLINE vec(vec && rhs) noexcept = default;
    RTS_ALWAYS_INLINE constexpr vec(std::tuple<Ts...> b) noexcept : data () {}

    RTS_ALWAYS_INLINE vec & operator=(const vec & rhs) noexcept = default;
    RTS_ALWAYS_INLINE vec & operator=(vec && rhs) noexcept = default;

    RTS_ALWAYS_INLINE RTS_CONST constexpr iterator begin() noexcept { return iterator(this); }
    RTS_ALWAYS_INLINE RTS_CONST constexpr iterator end() noexcept { return iterator(this+1); }
    RTS_ALWAYS_INLINE RTS_CONST constexpr const_iterator begin() const noexcept { return const_iterator(this); }
    RTS_ALWAYS_INLINE RTS_CONST constexpr const_iterator end() const noexcept { return const_iterator(this+1); }
    RTS_ALWAYS_INLINE RTS_CONST constexpr const_iterator cbegin() const noexcept { return const_iterator(this); }
    RTS_ALWAYS_INLINE RTS_CONST constexpr const_iterator cend() const noexcept { return const_iterator(this+1); }
    RTS_ALWAYS_INLINE RTS_CONST constexpr reference operator [] (int i) noexcept { return begin()[i]; }
    RTS_ALWAYS_INLINE RTS_CONST constexpr const_reference operator [] (int i) const noexcept { return cbegin()[i]; }
    
    RTS_ALWAYS_INLINE RTS_PURE constexpr auto get(int i) noexcept {
      return detail::index_apply<std::tuple_size<value_type>{}>(
        [&](auto... Is) { return std::make_tuple(std::get<Is>(data).get(i)...); }
      );
    }

    RTS_ALWAYS_INLINE RTS_PURE constexpr auto get(int i) const noexcept {
      return detail::index_apply<std::tuple_size<value_type>{}>(
        [&](auto... Is) { return std::make_tuple(std::get<Is>(data).get(i)...); }
      );
    }

    RTS_ALWAYS_INLINE void put(int i, const std::tuple<Ts...> & v) noexcept {
      detail::index_apply<std::tuple_size<value_type>{}>(
        [&](auto... Is) { RTS_UNUSED auto l = { detail::put1<Is>(data,i,v)... }; }
      );
    }
  };

  template <class T, class A>
  RTS_ALWAYS_INLINE RTS_PURE const vec<T&,A> operator * (const vec<T*,A> & ps) noexcept {
    return vec<T&,A>(ps, detail::indirection_tag);
  }

  template <class T, class A>
  RTS_ALWAYS_INLINE RTS_PURE vec<T&,A> operator * (vec<T*,A> & ps) noexcept {
    return vec<T&,A>(ps, detail::indirection_tag);
  }

  // -------------------------------------------------------------------------------
  // template implementation details
  // -------------------------------------------------------------------------------

#define RTS_OP(op) \
  template <class T, class A> \
  RTS_ALWAYS_INLINE RTS_PURE auto operator op (const vec<T, A> & l) noexcept { \
    vec<decltype(op l.get(0)),A> result; \
    for (int i =0;i<A::width;++i) result.put(i,op l.get(i)); \
    return result; \
  }

  RTS_OP(~)
  RTS_OP(!)
  RTS_OP(-)
  RTS_OP(+)

#undef RTS_OP

#define RTS_BINOP(op) \
  template <class T, class U, class A> \
  RTS_ALWAYS_INLINE RTS_PURE auto operator op (const vec<T, A> & l, const vec<U,A> & r) noexcept { \
    vec<decltype(l.get(0) op r.get(0)), A> result; \
    for (int i=0;i<A::width;++i) result.put(i, l.get(i) op r.get(i)); \
    return result; \
  } \
  template <class T, class U, class A> \
  RTS_ALWAYS_INLINE RTS_PURE auto operator op (const vec<T, A> & l, U r) noexcept { \
    vec<decltype(l.get(0) op r), A> result; \
    for (int i=0;i<A::width;++i) result.put(i, l.get(i) op r); \
    return result; \
  } \
  template <class T, class U, class A> \
  RTS_ALWAYS_INLINE RTS_PURE auto operator op (T l, const vec<U, A> & r) noexcept { \
    vec<decltype(l op r.get(0)), A> result; \
    for (int i=0;i<A::width;++i) result.put(i, l op r.get(i)); \
    return result; \
  }

  RTS_BINOP(+)
  RTS_BINOP(*)
  RTS_BINOP(-)
  RTS_BINOP(<<)
  RTS_BINOP(>>)
  RTS_BINOP(&)
  RTS_BINOP(|)
  RTS_BINOP(^)
  RTS_BINOP(&&)
  RTS_BINOP(||)
  RTS_BINOP(==)
  RTS_BINOP(!=)
  RTS_BINOP(<)
  RTS_BINOP(<=)
  RTS_BINOP(>)
  RTS_BINOP(>=)

#undef RTS_BINOP

  template <class S, class T, class A>
  S & operator << (S & os, const vec<T,A> & v) {
    os << '{';
    for (int i =0;i<A::width;++i) {
      os << ' ' << v.get(i);
      if (i != A::width - 1) os << ',';
    }
    return os << '}';
  }

  namespace detail {

    template <class T, class A>
    RTS_ALWAYS_INLINE RTS_PURE constexpr const_vptr<T, A> operator + (std::ptrdiff_t j, const_vptr<T,A> & rhs) noexcept {
      std::ptrdiff_t k(j + rhs.i);
      auto v(rhs.v + k / A::width);
      int32_t i(k % A::width);
      return (i < 0) ? const_vptr<T,A>(v-1,i+1) : const_vptr<T,A>(v,i);
    }

    template <class T, class A>
    RTS_ALWAYS_INLINE RTS_PURE constexpr const_vptr<T, A> operator + (const_vptr<T,A> & lhs, std::ptrdiff_t j) noexcept {
      std::ptrdiff_t k(lhs.i + j);
      auto v(lhs.v + k / A::width);
      int32_t i(k % A::width);
      return (i < 0) ? const_vptr<T,A>(v-1,i+1) : const_vptr<T,A>(v,i);
    }


    template <class T, class A>
    RTS_ALWAYS_INLINE RTS_PURE constexpr vptr<T, A> operator + (std::ptrdiff_t j, vptr<T,A> & rhs) noexcept {
      std::ptrdiff_t k(j + rhs.i);
      auto v(rhs.v + k / A::width);
      int32_t i(k % A::width);
      return (i < 0) ? vptr<T,A>(v-1,i+1) : vptr<T,A>(v,i);
    }

    template <class T, class A>
    RTS_ALWAYS_INLINE RTS_PURE constexpr vptr<T, A> operator + (vptr<T,A> & lhs, std::ptrdiff_t j) noexcept {
      std::ptrdiff_t k(lhs.i + j);
      auto v(lhs.v + k / A::width);
      int32_t i(k % A::width);
      return (i < 0) ? vptr<T,A>(v-1,i+1) : vptr<T,A>(v,i);
    }

    template <class T, class A>
    RTS_ALWAYS_INLINE RTS_PURE constexpr std::ptrdiff_t operator - (const const_vptr<T,A> & lhs, const const_vptr<T,A> & rhs) noexcept {
      return A::width * (lhs.v - rhs.v) + lhs.i - rhs.i;
    }

    template <class T, class A>
    RTS_ALWAYS_INLINE RTS_PURE constexpr bool operator == (const const_vptr<T,A> & lhs, const const_vptr<T,A> & rhs) noexcept {
      return lhs.v == rhs.v && lhs.i == rhs.i;
    }

    template <class T, class A>
    RTS_ALWAYS_INLINE RTS_PURE constexpr bool operator != (const const_vptr<T,A> & lhs, const const_vptr<T,A> & rhs) noexcept {
      return lhs.v != rhs.v || lhs.i != rhs.i;
    }

    template <class T, class A>
    RTS_ALWAYS_INLINE RTS_PURE constexpr bool operator == (const const_vptr<T,A> & lhs, std::nullptr_t) noexcept {
      return lhs.v == nullptr && lhs.i == 0;
    }

    template <class T, class A>
    RTS_ALWAYS_INLINE RTS_PURE constexpr bool operator == (std::nullptr_t, const const_vptr<T,A> & rhs) noexcept {
      return rhs.v == nullptr && rhs.i == 0;
    }

    template <class T, class A>
    RTS_ALWAYS_INLINE RTS_PURE constexpr bool operator != (const const_vptr<T,A> & lhs, std::nullptr_t) noexcept {
      return lhs.v != nullptr || lhs.i != 0;
    }

    template <class T, class A>
    RTS_ALWAYS_INLINE RTS_PURE constexpr bool operator != (std::nullptr_t, const const_vptr<T,A> & rhs) noexcept {
      return rhs.v != nullptr || rhs.i != 0;
    }

    template <class T, class A>
    RTS_ALWAYS_INLINE RTS_PURE constexpr bool operator < (const const_vptr<T,A> & lhs, const const_vptr<T,A> & rhs) noexcept {
      return lhs.v < rhs.v || (lhs.v == rhs.v && lhs.i < rhs.i);
    }

    template <class T, class A>
    RTS_ALWAYS_INLINE RTS_PURE constexpr bool operator < (std::nullptr_t, const const_vptr<T,A> & rhs) noexcept {
      return std::less<vec<T,A> const *>(nullptr, rhs.v) || (std::equal<vec<T,A> const *>(nullptr, rhs.v) && 0 < rhs.i);
    }

    template <class T, class A>
    RTS_ALWAYS_INLINE RTS_PURE constexpr bool operator < (const const_vptr<T,A> & lhs, std::nullptr_t) noexcept {
      return std::less<vec<T,A> const *>(lhs.v, nullptr) || (std::equal<vec<T,A> const *>(lhs.v, nullptr) && lhs.i < 0);
    }

    template <class T, class A>
    RTS_ALWAYS_INLINE RTS_PURE constexpr bool operator <= (const const_vptr<T,A> & lhs, const const_vptr<T,A> & rhs) noexcept {
      return lhs.v < rhs.v || (lhs.v == rhs.v && lhs.i <= rhs.i);
    }

    template <class T, class A>
    RTS_ALWAYS_INLINE RTS_PURE constexpr bool operator <= (const const_vptr<T,A> & lhs, std::nullptr_t) noexcept {
      return std::less<vec<T,A> const *>(lhs.v, nullptr) || (std::equal<vec<T,A> const *>(lhs.v, nullptr) && lhs.i <= 0);
    }

    template <class T, class A>
    RTS_ALWAYS_INLINE RTS_PURE constexpr bool operator <= (std::nullptr_t, const const_vptr<T,A> & rhs) noexcept {
      return std::less<vec<T,A> const *>(nullptr, rhs.v) || (std::equal<vec<T,A> const *>(nullptr, rhs.v) && 0 <= rhs.i);
    }

    template <class T, class A>
    RTS_ALWAYS_INLINE RTS_PURE constexpr bool operator > (const const_vptr<T,A> & lhs, const const_vptr<T,A> & rhs) noexcept {
      return lhs.v > rhs.v || (lhs.v == rhs.v && lhs.i > rhs.i);
    }

    template <class T, class A>
    RTS_ALWAYS_INLINE RTS_PURE constexpr bool operator > (const const_vptr<T,A> & lhs, std::nullptr_t) noexcept {
      return std::greater<vec<T,A> const *>(lhs.v, nullptr) || (std::equal<vec<T,A> const *>(lhs.v, nullptr) && lhs.i > 0);
    }

    template <class T, class A>
    RTS_ALWAYS_INLINE RTS_PURE constexpr bool operator > (std::nullptr_t, const const_vptr<T,A> & rhs) noexcept {
      return std::greater<vec<T,A> const *>(nullptr, rhs.v) || (std::equal<vec<T,A> const *>(nullptr, rhs.v) && 0 > rhs.i);
    }

    template <class T, class A>
    RTS_ALWAYS_INLINE RTS_PURE constexpr bool operator >= (const const_vptr<T,A> & lhs, const const_vptr<T,A> & rhs) noexcept {
      return lhs.v > rhs.v || (lhs.v == rhs.v && lhs.i >= rhs.i);
    }

    template <class T, class A>
    RTS_ALWAYS_INLINE RTS_PURE constexpr bool operator >= (const const_vptr<T,A> & lhs, std::nullptr_t) noexcept {
      return std::greater<vec<T,A> const *>(lhs.v, nullptr) || (std::equal<vec<T,A> const *>(lhs.v, nullptr) && lhs.i >= 0);
    }

    template <class T, class A>
    RTS_ALWAYS_INLINE RTS_PURE constexpr bool operator >= (std::nullptr_t, const const_vptr<T,A> & rhs) noexcept {
      return std::greater<vec<T,A> const *>(nullptr, rhs.v) || (std::equal<vec<T,A> const *>(nullptr, rhs.v) && 0 >= rhs.i);
    }

    template <class T, class A>
    RTS_ALWAYS_INLINE const_vptr<T, A> & operator++(const_vptr<T,A> & lhs) noexcept {
      lhs.i += 1;
      if (lhs.i > A::width) { lhs.i = 0; ++lhs.v; }
      return lhs;
    }

    template <class T, class A>
    RTS_ALWAYS_INLINE const_vptr<T, A> & operator--(const_vptr<T,A> & lhs) noexcept {
      lhs.i -= 1;
      if (lhs.i < 0 ) { lhs.i = A::width-1; --lhs.v; }
      return lhs;
    }

    template <class T, class A>
    RTS_ALWAYS_INLINE const_vptr<T, A> operator++(const_vptr<T,A> & lhs,int) noexcept {
      const_vptr<T,A> result = lhs;
      ++lhs;
      return result;
    }

    template <class T, class A>
    RTS_ALWAYS_INLINE const_vptr<T, A> operator--(const_vptr<T,A> & lhs,int) noexcept {
      const_vptr<T,A> result = lhs;
      --lhs;
      return result;
    }

    template <class T, class A>
    RTS_ALWAYS_INLINE vptr<T, A> & operator++(const_vptr<T,A> & lhs) noexcept {
      lhs.i += 1;
      if (lhs.i > A::width) { lhs.i = 0; ++lhs.v; }
      return lhs;
    }

    template <class T, class A>
    RTS_ALWAYS_INLINE vptr<T, A> & operator--(const_vptr<T,A> & lhs) noexcept {
      lhs.i -= 1;
      if (lhs.i < 0 ) { lhs.i = A::width-1; --lhs.v; }
      return lhs;
    }

    template <class T, class A>
    RTS_ALWAYS_INLINE vptr<T, A> operator++(const_vptr<T,A> & lhs,int) noexcept {
      vptr<T,A> result = lhs;
      ++lhs;
      return result;
    }

    template <class T, class A>
    RTS_ALWAYS_INLINE vptr<T, A> operator--(const_vptr<T,A> & lhs,int) noexcept {
      vptr<T,A> result = lhs;
      --lhs;
      return result;
    }

    template <class T, class A>
    RTS_ALWAYS_INLINE const_vptr<T,A> & const_vptr<T,A>::operator +=(std::ptrdiff_t j) noexcept {
      std::ptrdiff_t k = i + j;
      v += k / A::width;
      i  = static_cast<int32_t>(k % A::width);
      if (i < 0) { ++i; --v; }
      return *this;
    }

    template <class T, class A>
    RTS_ALWAYS_INLINE const_vptr<T,A> & const_vptr<T,A>::operator -=(std::ptrdiff_t j) noexcept {
      std::ptrdiff_t k = i - j;
      v += k / A::width;
      i  = static_cast<int32_t>(k % A::width);
      if (i < 0) { ++i; --v; }
      return *this;
    }

    template <class T, class A>
    RTS_ALWAYS_INLINE vptr<T,A> & vptr<T,A>::operator +=(std::ptrdiff_t j) noexcept {
      std::ptrdiff_t k = i + j;
      v += k / A::width;
      i  = static_cast<int32_t>(k % A::width);
      if (i < 0) { ++i; --v; }
      return *this;
    }

    template <class T, class A>
    RTS_ALWAYS_INLINE vptr<T,A> & vptr<T,A>::operator -=(std::ptrdiff_t j) noexcept {
      std::ptrdiff_t k = i - j;
      v += k / A::width;
      i  = static_cast<int32_t>(k % A::width);
      if (i < 0) { ++i; --v; }
      return *this;
    }

    template <size_t i, class T, class A>
    RTS_ALWAYS_INLINE void put(vec<T,A> & lhs, const T & rhs) noexcept {
      static_assert(i < A::width, "index out of bounds");
      lhs.put(i, rhs);
    }
  }

  using std::get;
  using std::tuple_size;
  using std::tuple_element;
} // namespace rts

namespace std {

  template <std::size_t i, class T, class A>
  RTS_ALWAYS_INLINE RTS_PURE auto get(rts::vec<T,A> & v) noexcept {
    static_assert(i < A::width, "index out of bounds");
    return v.get(i);
  }

  template <std::size_t i, class T, class A>
  RTS_ALWAYS_INLINE RTS_PURE auto get(const rts::vec<T,A> & v) noexcept {
    static_assert(i < A::width, "index out of bounds");
    return v.get(i);
  }

  template <class T, class A>
  class tuple_size<rts::vec<T,A>> : public integral_constant<size_t, A::width> {};

  template <std::size_t I, class T, class A>
  struct tuple_element<I, rts::vec<T,A> > {
    using type = T;
  };

  template <class T, class A>
  struct iterator_traits<rts::detail::vptr<T,A>> {
    using difference_type = ptrdiff_t;
    using value_type = T;
    using reference_type = rts::detail::vref<T,A>;
    using iterator_category = random_access_iterator_tag;
  };

  template <class T, class A>
  struct iterator_traits<rts::detail::const_vptr<T,A>> {
    using difference_type = ptrdiff_t;
    using value_type = T;
    using reference_type = rts::detail::const_vref<T,A>;
    using iterator_category = random_access_iterator_tag;
  };


  template <class T, class A>
  struct iterator_traits<rts::detail::vrefptr<T,A>> {
    using difference_type = ptrdiff_t;
    using value_type = T;
    using reference_type = typename rts::vec<T&,A>::reference;
    using iterator_category = random_access_iterator_tag;
  };
} // namespace std