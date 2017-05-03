#include "vec.hpp"


namespace rts {
  template <class A = default_isa>
  thread_local vec<bool,A> execution_mask = true;

  template <class T, class A = default_isa>
  struct varying {
    using vector = vec<T, A>;

    using iterator = typename vector::iterator;
    using const_iterator = typename vector::const_iterator;
    using pointer = typename vector::pointer;
    using const_pointer = typename vector::const_pointer;
    using reference = typename vector::reference;
    using const_reference = typename vector::const_reference;

    vector data;

    RTS_ALWAYS_INLINE constexpr varying() noexcept(std::is_nothrow_default_constructible<vector>::value) : data() {}

    template <typename ... Args>
    RTS_ALWAYS_INLINE constexpr varying(Args && ... args) noexcept(noexcept(vector(std::forward(args)...))) : data(std::forward(args)...) {}

    explicit RTS_ALWAYS_INLINE varying(const vector & data) noexcept(std::is_nothrow_copy_constructible<vector>::value) : data(data) {}
    explicit RTS_ALWAYS_INLINE varying(vector && data) noexcept(std::is_nothrow_move_constructible<vector>::value) : data(std::move(data)) {}

    RTS_ALWAYS_INLINE varying(varying && rhs) : data(std::move(rhs.data)) {}

    template <typename U>
    RTS_ALWAYS_INLINE varying & operator = (U rhs) noexcept {
        load(data,rhs,execution_mask<A>);
        return *this;
    }

    RTS_ALWAYS_INLINE RTS_CONST RTS_MUTABLE_CONSTEXPR iterator begin() noexcept { return data.begin(); }
    RTS_ALWAYS_INLINE RTS_CONST RTS_MUTABLE_CONSTEXPR iterator end() noexcept { return data.end(); }
    RTS_ALWAYS_INLINE RTS_CONST constexpr const_iterator begin() const noexcept { return data.begin(); }
    RTS_ALWAYS_INLINE RTS_CONST constexpr const_iterator end() const noexcept { return data.end(); }
    RTS_ALWAYS_INLINE RTS_CONST constexpr const_iterator cbegin() const noexcept { return data.cbegin(); }
    RTS_ALWAYS_INLINE RTS_CONST constexpr const_iterator cend() const noexcept { return data.cend(); }
    RTS_ALWAYS_INLINE RTS_CONST constexpr reference operator[](int i) noexcept { return data[i]; }
    RTS_ALWAYS_INLINE RTS_CONST constexpr const_reference operator[](int i) const noexcept { return data[i]; }
    RTS_ALWAYS_INLINE RTS_PURE RTS_MUTABLE_CONSTEXPR auto get(int i) noexcept { return data.get(i); }
    RTS_ALWAYS_INLINE RTS_PURE constexpr auto get(int i) const noexcept { return data.get(i); }
    RTS_ALWAYS_INLINE void put(int i, const T & rhs) noexcept { data.put(i,rhs); }
    RTS_ALWAYS_INLINE RTS_CONST RTS_MUTABLE_CONSTEXPR operator vec<T,A> & () & { return data; }
    RTS_ALWAYS_INLINE RTS_CONST constexpr operator const vec<T,A> & () const & { return data; }
    RTS_ALWAYS_INLINE RTS_CONST operator vec<T,A> && () && { return data; }
  };

  template <size_t i, class T, class A>
  RTS_ALWAYS_INLINE void put(varying<T,A> & v, const T & r) noexcept(noexcept(v.put(i,r))) {
    static_assert(i < A::width,"index out of bounds");
    return v.put(i,r);
  }

  template <class T, class A> 
  RTS_ALWAYS_INLINE RTS_PURE varying<T,A> make_varying(const vec<T,A> & v) noexcept(std::is_nothrow_copy_constructible<vec<T,A>>::value) {
    return varying<T,A>(v);        
  }

  template <class T, class A> 
  RTS_ALWAYS_INLINE varying<T,A> make_varying(vec<T,A> && v) noexcept(std::is_nothrow_move_constructible<vec<T,A>>::value) {
    return varying<T,A>(std::move(v));
  }

  #define RTS_UNARY_MATH(fun) \
    template <class T, class A> \
    RTS_ALWAYS_INLINE RTS_MATH_PURE constexpr auto fun(const varying<T,A> & v) RTS_MATH_NOEXCEPT { \
      return make_varying(fun(vec<T,A>(v))); \
    }

  #define RTS_BINARY_MATH(fun) \
    template <class U, class V, class A> \
    RTS_ALWAYS_INLINE RTS_MATH_PURE constexpr auto fun(const varying<U,A> & u, const varying<V,A> & v) RTS_MATH_NOEXCEPT { \
      return make_varying(fun(vec<U,A>(v), vec<V,A>(v))); \
    }

  #define RTS_TERNARY_MATH(fun) \
    template <class U, class V, class W, class A> \
    RTS_ALWAYS_INLINE RTS_MATH_PURE constexpr auto fun(const varying<U,A> & u, const varying<V,A> & v, const varying<W,A> & w) RTS_MATH_NOEXCEPT { \
      return make_varying(fun(vec<U,A>(v), vec<V,A>(v), vec<W,A>(w))); \
    }

  #include "x-math.hpp"


  namespace detail {
    template <class A>
    struct execution_mask_scope {
      vec<bool,A> old_mask;

      RTS_ALWAYS_INLINE execution_mask_scope() noexcept : old_mask(execution_mask<A>) {}
      execution_mask_scope(const execution_mask_scope & other) = delete;
      execution_mask_scope(execution_mask_scope && other) = delete;
      RTS_ALWAYS_INLINE ~execution_mask_scope() noexcept { execution_mask<A> = old_mask; }
    };
  }

  template <class T, class A>
  RTS_ALWAYS_INLINE void if_(const vec<bool,A> & v, T t) noexcept(noexcept(t())) {
    detail::execution_mask_scope<A> scope;
    if (any(execution_mask<A> &= v))
      t();
  }

  template <class T, class F, class A>
  RTS_ALWAYS_INLINE void if_(const vec<bool,A> & v, T t, F f) noexcept(noexcept(t()) && noexcept(f())) {
    detail::execution_mask_scope<A> scope;
    if (any(execution_mask<A> &= v))
      t();
    if (any(execution_mask<A> = ~execution_mask<A> & scope.old_mask))
      f();
  }

  template <class T>
  RTS_ALWAYS_INLINE void if_(bool v, T t) noexcept(noexcept(t())) {
    if (v) t();
  }

  template <class T, class F>
  RTS_ALWAYS_INLINE void if_(bool v, T t, F f) noexcept(noexcept(t()) && noexcept(f())) {
    if (v) t(); else f();
  }
} // namespace rts

namespace std {
  template <std::size_t i, class T, class A>
  RTS_ALWAYS_INLINE RTS_PURE auto get(rts::varying<T,A> & v) noexcept(noexcept(v.get(i))) {
    static_assert(i < A::width,"index out of bounds");
    return v.get(i);
  }
  template <std::size_t i, class T, class A>
  RTS_ALWAYS_INLINE RTS_PURE auto get(const rts::varying<T,A> & v) noexcept(noexcept(v.get(i))) {
    static_assert(i < A::width,"index out of bounds");
    return v.get(i);
  }

  template <class T, class A>
  class tuple_size<rts::varying<T,A>> : public integral_constant<size_t, A::width> {};

  template <std::size_t I, class T, class A>
  struct tuple_element<I, rts::varying<T,A> > {
    using type = T;
  };

  template <class T, class A>
  struct numeric_limits<rts::varying<T,A>> {
    using base_limits = std::numeric_limits<rts::vec<T,A>>;
    static constexpr bool is_specialized = true;
    static constexpr bool is_signed = base_limits::is_signed; // ?
    static constexpr bool is_integer = base_limits::is_integer; // ?
    static constexpr bool is_exact = base_limits::is_exact;
    static constexpr bool has_infinity = base_limits::has_infinity;
    static constexpr bool has_quiet_NaN = base_limits::has_quiet_NaN;
    static constexpr bool has_signaling_NaN = base_limits::has_signaling_NaN;
    static constexpr std::float_denorm_style has_denorm = base_limits::has_denorm;
    static constexpr bool has_denorm_loss = base_limits::has_denorm_loss;
    static constexpr std::float_round_style round_style = base_limits::round_style;
    static constexpr bool is_iec559 = base_limits::iec559;
    static constexpr bool is_bounded = base_limits::is_bounded;
    static constexpr bool is_modulo = base_limits::is_modulo;
    static constexpr int digits = base_limits::digits;
    static constexpr int digits10 = base_limits::digits10;
    static constexpr int max_digits10 = base_limits::max_digits10;
    static constexpr int radix = base_limits::radix;
    static constexpr int min_exponent = base_limits::min_exponent;
    static constexpr int max_exponent = base_limits::max_exponent;
    static constexpr int min_exponent10 = base_limits::min_exponent10;
    static constexpr int max_exponent10 = base_limits::max_exponent10;
    static constexpr bool traps = base_limits::traps;
    static constexpr bool tinyness_before = base_limits::tinyness_before;
    static RTS_MATH_PURE constexpr rts::varying<T,A> max() RTS_MATH_NOEXCEPT { return rts::varying<T,A>(base_limits::max()); }
    static RTS_MATH_PURE constexpr rts::varying<T,A> min() RTS_MATH_NOEXCEPT { return rts::varying<T,A>(base_limits::min()); }
    static RTS_MATH_PURE constexpr rts::varying<T,A> lowest() RTS_MATH_NOEXCEPT { return rts::varying<T,A>(base_limits::lowest()); }
    static RTS_MATH_PURE constexpr rts::varying<T,A> epsilon() RTS_MATH_NOEXCEPT { return rts::varying<T,A>(base_limits::epsilon()); }
    static RTS_MATH_PURE constexpr rts::varying<T,A> round_error() RTS_MATH_NOEXCEPT { return rts::varying<T,A>(base_limits::round_error()); }
    static RTS_MATH_PURE constexpr rts::varying<T,A> infinity() RTS_MATH_NOEXCEPT { return rts::varying<T,A>(base_limits::infinity()); }
    static RTS_MATH_PURE constexpr rts::varying<T,A> quiet_NaN() RTS_MATH_NOEXCEPT { return rts::varying<T,A>(base_limits::quiet_NaN()); }
    static RTS_MATH_PURE constexpr rts::varying<T,A> signaling_NaN() RTS_MATH_NOEXCEPT { return rts::varying<T,A>(base_limits::signaling_NaN()); }
    static RTS_MATH_PURE constexpr rts::varying<T,A> denorm_min() RTS_MATH_NOEXCEPT { return rts::varying<T,A>(base_limits::signaling_NaN()); }
  };
} // namespace std