#pragma once

#include <chrono>
#include <type_traits>

/// @file chrono.hpp
/// @brief Portable support for @p std::chrono extras

namespace rts {
  namespace chrono {
    /// @cond PRIVATE
    namespace detail {
      template <class T> struct is_duration : std::false_type {};
      template <class Rep, class Period> struct is_duration<std::chrono::duration<Rep, Period>> : std::true_type {};

      template <class To, class Rep, class Period> constexpr To floor_helper(const std::chrono::duration<Rep, Period>& d, To t) {
        return t > d ?  t - To{1} : t;
      }
    }
    /// @endcond

    /// @brief round a floating point duration down towards the floor to form an integral duration
    ///
    /// Portable support for c++17's @p std::chrono::floor
    template <class To, class Rep, class Period, class = typename std::enable_if<detail::is_duration<To>{}>::type> constexpr To floor(const std::chrono::duration<Rep, Period>& d) {
      return detail::floor_helper<To,Rep,Period>(d, std::chrono::duration_cast<To>(d));
    }
  }
}
