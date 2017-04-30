#include <boost/context/all.hpp>
#include <exception>
#include <memory>
#include <type_traits>


namespace rts {
  namespace enumerators {
    // --------------------------------------------------------------------------------
    // enumerator expressions
    // --------------------------------------------------------------------------------

    // forward declarations
    namespace detail {
      template <typename T, typename F, typename A, typename B> struct then_enumerator;
      template <typename T, typename P, typename A> struct where_enumerator;
      template <typename T, typename F, typename A, typename B> struct map_enumerator;

      template <typename T, typename A> struct enumerator_expr {
        template <typename F> void foreach(F f) { return static_cast<T&>(*this).foreach(f); }
        template <typename F> auto then(F f) -> detail::then_enumerator<T,F,A,decltype(f(std::declval<A>()))>;
        template <typename F> auto map(F f) -> detail::map_enumerator<T,F,A,decltype(f(std::declval<A>()))>;
        template <typename P> detail::where_enumerator<T,P,A> where(P);

        operator T & () { return static_cast<T&>(*this); }
        operator T const & () const { return static_cast<const T&>(*this); }
      };

      template <typename T, typename F, typename A, typename B> struct then_enumerator : enumerator_expr<then_enumerator<T,F,A,B>,B> {
        T m;
        F f;
        then_enumerator() = default;
        then_enumerator(const then_enumerator &) = default;
        then_enumerator(then_enumerator &&) = default;
        then_enumerator(const T & m, const F & f) : m(m), f(f) {}
        template <typename G> void foreach(G g) {
          m.foreach([&](A a){ f(a).foreach(g); });
        }
      };

      template <typename T, typename P, typename A> struct where_enumerator : enumerator_expr<where_enumerator<T,P,A>,A> {
        T m;
        P p;
        where_enumerator() = default;
        where_enumerator(const where_enumerator &) = default;
        where_enumerator(where_enumerator &&) = default;
        where_enumerator(const T & m, const P & p) : m(m), p(p) {}
        template <typename G> void foreach(G g) {
          m.foreach([&](A a) { if (p(a)) g(a); });
        }
      };

      template <typename T, typename F, typename A, typename B> struct map_enumerator : enumerator_expr<map_enumerator<T,F,A,B>,B> {
        T m;
        F f;
        map_enumerator() = default;
        map_enumerator(const map_enumerator &) = default;
        map_enumerator(map_enumerator &&) = default;
        map_enumerator(const T & m, const F & f) : m(m), f(f) {}
        template <typename G> void foreach(G g) {
          m.foreach([&](A a) { g(f(a)); });
        }
      };
    
      template <typename T, typename A> template <typename P> where_enumerator<T,P,A> enumerator_expr<T,A>::where(P p) {
        return where_enumerator<T,P,A>(*this,p);
      }
      template <typename T, typename A> template <typename F> auto enumerator_expr<T,A>::then(F f) -> then_enumerator<T,F,A,decltype(f(std::declval<A>()))> {
        return then_enumerator<T,F,A,decltype(f(A()))>(*this,f);
      }
      template <typename T, typename A> template <typename F> auto enumerator_expr<T,A>::map(F f) -> map_enumerator<T,F,A,decltype(f(std::declval<A>()))> {
        return map_enumerator<T,F,A,decltype(f(A()))>(*this,f);
      }
    }

    // --------------------------------------------------------------------------------
    // example expressions
    // --------------------------------------------------------------------------------


    template <typename A> struct empty : detail::enumerator_expr<empty<A>,A> {
      template <typename F> void foreach(F f) const {}
    };

    namespace detail {
      template <typename A> struct range_lt_enumerator : enumerator_expr<range_lt_enumerator<A>,A> {
        A lo, hi;
        range_lt_enumerator(A lo, A hi) : lo(lo), hi(hi){}
        template <typename F> void foreach(F f) const {
          for (A i = lo;i<hi;++i) f(i);
        }
      };
    }

    template <typename A> auto range_lt(A lo, A hi) { return detail::range_lt_enumerator<A>(lo,hi); }

    namespace detail {
      template <typename A> struct range_le_enumerator : enumerator_expr<range_le_enumerator<A>,A> {
        A lo, hi;
        range_le_enumerator(A lo, A hi) : lo(lo), hi(hi){}
        template <typename F> void foreach(F f) const {
          for (A i = lo;i<hi;++i) f(i);
        }
      };
    }

    template <typename A> auto range_le(A lo, A hi) { return detail::range_le_enumerator<A>(lo,hi); }

    namespace detail {
      template <typename A> struct from_enumerator : enumerator_expr<from_enumerator<A>,A> {
        A lo;
        from_enumerator(A lo) : lo(lo) {}
        template <typename F> void foreach(F f) const {
          for (A i = lo;;++i) f(i);
        }
      };
    }

    template <typename A> auto from(A lo) { return detail::from_enumerator<A>(lo); }

    namespace detail {
      template <typename A, typename E> struct each_enumerator : detail::enumerator_expr<each_enumerator<A,E>,E> {
        A lo, hi;
        each_enumerator(A lo, A hi) : lo(lo), hi(hi){}
        template <typename F> void foreach(F f) const {
          for (A i = lo;i<hi;++i) f(*i);
        }
      };
    }

    // used for iterators
    template <typename A> auto each(A lo, A hi) { return detail::each_enumerator<A,decltype(*std::declval<A>())>(lo,hi); }
    template <typename T> auto each(T container) {
      return detail::each_enumerator<decltype(container.begin()), decltype(*container.begin())>(container.begin(),container.end());
    }

    // --------------------------------------------------------------------------------
    // Tasks
    // --------------------------------------------------------------------------------

    namespace detail {
      template <typename R, typename ... Args> struct task_base {
        // task_base() = delete;
        // task_base(const task_base & other) = delete;
        virtual R operator ()(Args...) = 0;
        virtual ~task_base() {}
      };

      template <typename F, typename R, typename ... Args> struct task_impl : task_base<R,Args...> {
        F f;
        task_impl(F f) : f(f) {}
        virtual R operator ()(Args... args) {
          return f(args...);
        }
      };

    

      // one-shot std::function, only requires move construction
      template <typename> struct task;
      template <typename R, typename ... Args> struct task<R(Args...)> {
        template <typename F> task(F f) : task(f,typename std::integral_constant<bool, std::is_convertible<F,bool>::value>::type()) {}
        task() : impl () {}
        task(std::nullptr_t) : impl() {}
        task(const task & t) = delete;
        R operator ()(Args... args) { return (*impl)(args...); }
        operator bool () const { return impl; }
        typedef R result_type;
        void swap(task & other) {
          std::swap(impl,other.impl);
        }
        task & operator = (task && rhs) {
          impl = std::move(rhs.impl);
          return *this;
        }
        std::unique_ptr<detail::task_base<R,Args...>> impl;
      private:
        template <typename F> task(F f, std::false_type) : impl(new detail::task_impl<F,R,Args...>(f)) {}         // not bool convertible
        template <typename F> task(F f, std::true_type) : impl(f?new detail::task_impl<F,R,Args...>(f):nullptr) {} // bool convertible, check if null
      };

      template <typename R, typename ... Args> bool operator == (const task<R(Args...)> & t, std::nullptr_t) { return !t; }
      template <typename R, typename ... Args> bool operator == (std::nullptr_t, const task<R(Args...)> & t) { return !t; }
      template <typename R, typename ... Args> bool operator != (const task<R(Args...)> & t, std::nullptr_t) { return bool(t); }
      template <typename R, typename ... Args> bool operator != (std::nullptr_t, const task<R(Args...)> & t) { return bool(t); }
    }

    // --------------------------------------------------------------------------------
    // enumerators
    // --------------------------------------------------------------------------------

    template <typename A, typename stack_allocator = boost::context::protected_fixedsize_stack> struct enumerator {
      enum status : intptr_t { complete = 0, next, bad };
      template <typename T> enumerator(detail::enumerator_expr<T,A> && e)
        : allocator()
        , sp(allocator.allocate())
        , last {}
        , g(boost::context::detail::make_fcontext(sp.sp,sp.size,exec)) {
          body = [&e, this](boost::context::detail::fcontext_t m) {
            try {
              e.foreach([&](A a) {
                new (&last.a) A(a); // placement new
                m = boost::context::detail::jump_fcontext(m, reinterpret_cast<void*>(status::next)).fctx;
                last.a.~A(); // placement delete
              });
            } catch(...) {
              new (&last.e) std::exception_ptr(std::current_exception()); // placement new
              m = boost::context::detail::jump_fcontext(m, reinterpret_cast<void*>(status::bad)).fctx;
              assert(false);
            }
            m = boost::context::detail::jump_fcontext(m, reinterpret_cast<void*>(status::complete)).fctx;
        };
      }
      ~enumerator() {
        if (sp.sp != nullptr) allocator.deallocate(sp);
      }
      // override default
      enumerator() : enumerator(empty<A>()) {}

      // disable copy, copy-assignment
      enumerator(const enumerator & e) = delete;
      enumerator & operator = (const enumerator & e) = delete;

      // default move, move-assignment
      enumerator(enumerator && e)
      : g(std::move(e.g))
      , allocator(std::move(e.allocator))
      , sp(std::move(e.sp))
      , body(std::move(e.body)) {
        e.sp.sp = nullptr;

      }
      enumerator & operator = (enumerator && e) {
        g = std::move(e.g);
        allocator = std::move(e.allocator);
        sp = std::move(e.sp);
        body = std::move(e.body);
        e.sp.sp = nullptr;
      }

      void swap(enumerator & e) {
        std::swap(g,e.g);
        std::swap(allocator,e.allocator);
        std::swap(sp,e.sp);
        std::swap(body,e.body);
      }

      bool valid() const noexcept { return sp.sp != nullptr; }

      template <typename F> void foreach(F f) {
        if (sp.sp == nullptr) return;
        struct cleanup { // local raii cleanup type
          stack_allocator & allocator;
          boost::context::stack_context sp;
          ~cleanup() { allocator.deallocate(sp); }
        } finally { allocator,sp };
        sp.sp = nullptr;
        boost::context::detail::transfer_t t = boost::context::detail::jump_fcontext(g,reinterpret_cast<void*>(&body));
        int i = static_cast<status>(reinterpret_cast<intptr_t>(t.data));
        while (i == status::next) {
          try {
            f(last.a);
          } catch (...) {
            last.a.~A();
            throw;
          }
          t = boost::context::detail::jump_fcontext(t.fctx,reinterpret_cast<void*>(&body));
          i = static_cast<status>(reinterpret_cast<intptr_t>(t.data));
        }
        g = t.fctx;
        if (i == status::bad) {
          std::exception_ptr e = last.e;
          last.e.~exception_ptr();
          std::rethrow_exception(e);
        }
      }

      template <typename F, typename B> detail::then_enumerator<enumerator,F,A,B> then(F f) { return detail::then_enumerator<enumerator,F,A,B>(*this,f); }
      template <typename P> detail::where_enumerator<enumerator,P,A> where(P p) { return detail::where_enumerator<enumerator,P,A>(*this,p); }
      template <typename F, typename B> detail::map_enumerator<enumerator,F,A,B> map(F f) { return detail::map_enumerator<enumerator,F,A,B>(*this,f); }

    private:
      struct unit {};
      union landing_pad {
        unit u;
        A a;
        std::exception_ptr e;
        ~landing_pad() {} // delegated to the surrounding object
      } last;
      stack_allocator allocator;
      boost::context::stack_context sp;
      boost::context::detail::fcontext_t g;
      detail::task<void(boost::context::detail::fcontext_t)> body; // for a move only function, this is more than we need, fix that.
      static void exec(boost::context::detail::transfer_t p) {
        (*reinterpret_cast<detail::task<void(boost::context::detail::fcontext_t)>*>(p.data))(p.fctx);
      }
    };
  }
}

#ifdef TEST_GENERATOR // example

#include <iostream>

using namespace rts::enumerators;

int main(int argc, char ** argv) {
  range_lt(1,5).map([&](auto a) { return a + 1; }).where([&] (int i) { return i % 2 == 0;}).foreach([&](auto i) {
    std::cout << i << "\n";
  });

  enumerator<int> g(range_lt<int>(1,5));
  g.foreach([&](auto i) {
    std::cout << i << "\n";
  });
}
#endif
