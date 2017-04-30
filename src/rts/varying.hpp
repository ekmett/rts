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

    RTS_ALWAYS_INLINE constexpr varying() noexcept : data() {}

    template <typename ... Args>
    RTS_ALWAYS_INLINE constexpr varying(Args ... args) noexcept : data(std::forward(args)...) {}

    RTS_ALWAYS_INLINE varying(varying && rhs) : data(std::move(rhs.data)) {}

    template <typename U>
    RTS_ALWAYS_INLINE varying & operator = (U rhs) noexcept {
        load(data,rhs,execution_mask<A>);
        return *this;
    }

    RTS_ALWAYS_INLINE RTS_CONST constexpr iterator begin() noexcept { return data.begin(); }
    RTS_ALWAYS_INLINE RTS_CONST constexpr iterator end() noexcept { return data.end(); }
    RTS_ALWAYS_INLINE RTS_CONST constexpr const_iterator begin() const noexcept { return data.begin(); }
    RTS_ALWAYS_INLINE RTS_CONST constexpr const_iterator end() const noexcept { return data.end(); }
    RTS_ALWAYS_INLINE RTS_CONST constexpr const_iterator cbegin() const noexcept { return data.cbegin(); }
    RTS_ALWAYS_INLINE RTS_CONST constexpr const_iterator cend() const noexcept { return data.cend(); }
    RTS_ALWAYS_INLINE RTS_CONST constexpr reference operator[](int i) noexcept { return data[i]; }
    RTS_ALWAYS_INLINE RTS_CONST constexpr const_reference operator[](int i) const noexcept { return data[i]; }
    RTS_ALWAYS_INLINE RTS_PURE constexpr auto get(int i) noexcept { return data.get(i); }
    RTS_ALWAYS_INLINE RTS_PURE constexpr auto get(int i) const noexcept { return data.get(i); }
    RTS_ALWAYS_INLINE void put(int i, const T & rhs) noexcept { data.put(i,rhs); }
  };

  template <size_t i, class T, class A>
  RTS_ALWAYS_INLINE void put(varying<T,A> & v, const T & r) noexcept {
    static_assert(i < A::width,"index out of bounds");
    return v.put(i,r);
  }

  template <class A>
  struct execution_mask_scope {
    vec<bool,A> old_mask;

    RTS_ALWAYS_INLINE execution_mask_scope() : old_mask(execution_mask<A>) {}
    execution_mask_scope(const execution_mask_scope & other) = delete;
    execution_mask_scope(execution_mask_scope && other) = delete;
    RTS_ALWAYS_INLINE ~execution_mask_scope() { execution_mask<A> = old_mask; }
  };

  template <class T, class A>
  RTS_ALWAYS_INLINE void if_(const vec<bool,A> & v, T t) {
    execution_mask_scope<A> scope;
    if (any(execution_mask<A> &= v))
      t();
  }

  template <class T, class F, class A>
  RTS_ALWAYS_INLINE void if_(const vec<bool,A> & v, T t, F f) {
    execution_mask_scope<A> scope;
    if (any(execution_mask<A> &= v))
      t();
    if (any(execution_mask<A> = ~execution_mask<A> & scope.old_mask))
      f();
  }

  template <class T>
  RTS_ALWAYS_INLINE void if_(bool v, T t) {  
    if (v) t();
  }

  template <class T, class F>
  RTS_ALWAYS_INLINE void if_(bool v, T t, F f) {  
    if (v) t(); else f();
  }
} // namespace rts

namespace std {
  template <std::size_t i, class T, class A>
  RTS_ALWAYS_INLINE RTS_PURE auto get(rts::varying<T,A> & v) noexcept {
    static_assert(i < A::width,"index out of bounds");
    return v.get(i);
  }
  template <std::size_t i, class T, class A>
  RTS_ALWAYS_INLINE RTS_PURE auto get(const rts::varying<T,A> & v) noexcept {
    static_assert(i < A::width,"index out of bounds");
    return v.get(i);
  }

  template <class T, class A>
  class tuple_size<rts::varying<T,A>> : public integral_constant<size_t, A::width> {};

  template <std::size_t I, class T, class A>
  struct tuple_element<I, rts::varying<T,A> > {
    using type = T;
  };
} // namespace std