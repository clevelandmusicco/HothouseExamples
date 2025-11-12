/*=============================================================================
   Copyright (c) 2016-2023 Joel de Guzman

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(INFRA_SUPPORT_APRIL_10_2016)
#define INFRA_SUPPORT_APRIL_10_2016

#if defined(_WIN32)
# define _USE_MATH_DEFINES // for M_PI
# include <cmath>
#endif

#include <cfloat>
#include <cstdint>
#include <cassert>
#include <chrono>
#include <limits>
#include <climits>
#include <string>
#include <type_traits>
#include <memory>

namespace cycfi
{
   ////////////////////////////////////////////////////////////////////////////
   // Ignore utility for ignoring unused params.
   ////////////////////////////////////////////////////////////////////////////
   template <typename... T>
   inline void ignore(T&&...) {}

   ////////////////////////////////////////////////////////////////////////////
   // Constants
   ////////////////////////////////////////////////////////////////////////////
   constexpr auto pi = 3.14159265358979323846264338327f;

   ////////////////////////////////////////////////////////////////////////////
   // Non-copyable base class
   ////////////////////////////////////////////////////////////////////////////
   struct non_copyable
   {
                        non_copyable() = default;
                        non_copyable(non_copyable const& rhs) = delete;
                        ~non_copyable() = default;
      non_copyable&     operator=(non_copyable const&) = delete;
   };

   ////////////////////////////////////////////////////////////////////////////
   // Unused (parameter and variable) utility
   ////////////////////////////////////////////////////////////////////////////
   template <typename... T>
   void unused(T&&...) {}

   ////////////////////////////////////////////////////////////////////////////
   // Shared pointer utilities
   ////////////////////////////////////////////////////////////////////////////
   template <typename T>
   inline auto share(T&& obj)
   {
      using type = typename std::decay<T>::type;
      return std::make_shared<type>(std::forward<T>(obj));
   }

   template <typename T>
   inline auto get(std::shared_ptr<T> const& ptr)
   {
      return std::weak_ptr<T>(ptr);
   }

   ////////////////////////////////////////////////////////////////////////////
   // Time
   ////////////////////////////////////////////////////////////////////////////
   using duration       = std::chrono::duration<double>;
   using microseconds   = std::chrono::duration<double, std::micro>;
   using milliseconds   = std::chrono::duration<double, std::milli>;
   using seconds        = std::chrono::duration<double>;
   using minutes        = std::chrono::duration<double, std::ratio<60>>;
   using hours          = std::chrono::duration<double, std::ratio<60*60>>;
   using time_point     = std::chrono::time_point<std::chrono::steady_clock, duration>;

   ////////////////////////////////////////////////////////////////////////////
   // constexpr utilities
   ////////////////////////////////////////////////////////////////////////////
   constexpr bool equal(char const* lhs, char const* rhs)
   {
      if (lhs == rhs)
         return true;
      if (!lhs || !rhs)
         return false;
      while (*lhs || *rhs)
         if (*lhs++ != *rhs++)
            return false;
      return true;
   }

   ////////////////////////////////////////////////////////////////////////////
   // Metaprogramming utilities
   ////////////////////////////////////////////////////////////////////////////
   template <typename T>
   struct remove_cvref
   {
      using type = typename std::remove_cv<typename std::remove_reference<T>::type>::type;
   };

   template <typename T>
   using remove_cvref_t = typename remove_cvref<T>::type;

   template <typename T, typename... Rest>
   struct is_arithmetic
   {
      static constexpr bool value
         = std::is_arithmetic<T>::value && is_arithmetic<Rest...>::value;
   };

   template <typename T>
   struct is_arithmetic<T>
   {
      static constexpr bool value = std::is_arithmetic<T>::value;
   };

   namespace detail
   {
      template <std::size_t bits>
      struct int_that_fits_impl { using type = void; };

      template <>
      struct int_that_fits_impl<8> { using type = std::int8_t; };

      template <>
      struct int_that_fits_impl<16> { using type = std::int16_t; };

      template <>
      struct int_that_fits_impl<32> { using type = std::int32_t; };

      template <>
      struct int_that_fits_impl<64> { using type = std::int64_t; };

      template <std::size_t bits>
      struct uint_that_fits_impl { using type = void; };

      template <>
      struct uint_that_fits_impl<8> { using type = std::uint8_t; };

      template <>
      struct uint_that_fits_impl<16> { using type = std::uint16_t; };

      template <>
      struct uint_that_fits_impl<32> { using type = std::uint32_t; };

      template <>
      struct uint_that_fits_impl<64> { using type = uint64_t; };

      constexpr std::size_t size_that_fits_int(std::size_t bits)
      {
         if (bits <= 8)
            return 8;
         else if (bits <= 16)
            return 16;
         else if (bits <= 32)
            return 32;
         else if (bits <= 64)
            return 64;
         return 0;
      }
   }

   template <std::size_t bits>
   struct int_that_fits
     : detail::int_that_fits_impl<detail::size_that_fits_int(bits)>
   {
      using type = typename
         detail::int_that_fits_impl<detail::size_that_fits_int(bits)>::type;
#if !defined(_MSC_VER)
      static_assert(!std::is_same<type, void>::value,
         "Error: No int type fits specified number of bits."
      );
#endif
   };

   template <std::size_t bits>
   struct uint_that_fits
     : detail::uint_that_fits_impl<detail::size_that_fits_int(bits)>
   {
      using type = typename
         detail::uint_that_fits_impl<detail::size_that_fits_int(bits)>::type;
#if !defined(_MSC_VER)
      static_assert(!std::is_same<type, void>::value,
         "Error: No int type fits specified number of bits."
      );
#endif
   };

   using natural_int = typename int_that_fits<sizeof(void*) * CHAR_BIT>::type;
   using natural_uint = typename uint_that_fits<sizeof(void*) * CHAR_BIT>::type;

   ////////////////////////////////////////////////////////////////////////////
   // Constants
	////////////////////////////////////////////////////////////////////////////
   template <typename T>
   struct int_traits
   {
      static constexpr T max = std::numeric_limits<T>::max();
      static constexpr T min = std::numeric_limits<T>::min();
   };

   template <typename T>
   constexpr T int_max()
   {
      return int_traits<T>::max;
   }

   template <typename T>
   constexpr T int_min()
   {
      return int_traits<T>::min;
   }

   ////////////////////////////////////////////////////////////////////////////
   // integer and binary functions
   ////////////////////////////////////////////////////////////////////////////
   constexpr int16_t promote(int8_t i)
   {
      return i;
   }

   constexpr uint16_t promote(uint8_t i)
   {
      return i;
   }

   constexpr int32_t promote(int16_t i)
   {
      return i;
   }

   constexpr std::uint32_t promote(uint16_t i)
   {
      return i;
   }

   constexpr int64_t promote(int32_t i)
   {
      return i;
   }

   constexpr uint64_t promote(uint32_t i)
   {
      return i;
   }

   constexpr double promote(float i)
   {
      return i;
   }

   constexpr long double promote(double i)
   {
      return i;
   }

   template <typename T>
   constexpr T pow2(std::size_t n)
   {
      return (n == 0)? T(1) : T(2) * pow2<T>(n-1);
   }

   // This is needed to force compile-time evaluation
   template <typename T, size_t n>
   struct static_pow2
   {
      constexpr static T val = pow2<T>(n);
   };

   // smallest power of 2 that fits n
   template <typename T>
   constexpr T smallest_pow2(T n, T m = 1)
   {
      return (m < n)? smallest_pow2(n, m << 1) : m;
   }

   template <typename T>
   constexpr bool is_pow2(T n)
   {
      return (n & (n - 1)) == 0;
   }

   ////////////////////////////////////////////////////////////////////////////
   // static int types
	////////////////////////////////////////////////////////////////////////////
   template <typename T, T value_>
   struct static_int
   {
      static constexpr T value =  value_;
   };

   template <int i>
   using int_ = static_int<int, i>;

   template <std::size_t i>
   using uint_ = static_int<std::size_t, i>;

   template <int8_t i>
   using int8_ = static_int<int8_t, i>;

   template <uint8_t i>
   using uint8_ = static_int<uint8_t, i>;

   template <int16_t i>
   using int16_ = static_int<int16_t, i>;

   template <uint16_t i>
   using uint16_ = static_int<uint16_t, i>;

   template <int32_t i>
   using int32_ = static_int<int32_t, i>;

   template <uint32_t i>
   using uint32_ = static_int<uint32_t, i>;

   template <int64_t i>
   using int64_ = static_int<int64_t, i>;

   template <uint64_t i>
   using uint64_ = static_int<uint64_t, i>;

   ////////////////////////////////////////////////////////////////////////////
   // Utilities
   ////////////////////////////////////////////////////////////////////////////
   template <typename T, typename U>
   inline T& clamp_max(T& val, U const& max)
   {
      if (val > max)
         val = max;
      return val;
   }

   template <typename T, typename U>
   inline T& clamp_min(T& val, U const& min)
   {
      if (val < min)
         val = min;
      return val;
   }

   template <typename T, typename U, typename V>
   inline T& clamp(T& val, U const& min, V const& max)
   {
      assert(min <= max);
      clamp_min(val, min);
      clamp_max(val, max);
      return val;
   }

   template <typename T, typename U, typename V>
   inline bool within(T const& val, U const& min, V const& max)
   {
      return (val >= min) && (val <= max);
   }

   template <typename T>
   constexpr T abs(T i)
   {
      return (i >= 0)? i : -i;
   }

   ////////////////////////////////////////////////////////////////////////////
   // deleter: generic custom deleter for, e.g. unique_ptr.
   ////////////////////////////////////////////////////////////////////////////
   template <typename T, void(&delete_)(T*)>
   struct deleter
   {
      void operator()(T* p) { delete_(p); }
   };

   ////////////////////////////////////////////////////////////////////////////
   // Return true if little endian
   ////////////////////////////////////////////////////////////////////////////
   inline bool is_little_endian()
   {
      static_assert(sizeof(char)!=sizeof(short), "Error: not usable on this machine");
      short number = 0x1;
      char *p = reinterpret_cast<char*>(&number);
      return (p[0] == 1);
   }

   //==============================================================================================
   /// @brief Evaluates to `T` if `T` is a scalar type or to `T const&` if not.
   /// @tparam T The original type parameter.
   //==============================================================================================
   template <typename T>
   using param_type =
      std::conditional_t<std::is_scalar_v<T>, T, std::remove_reference_t<T> const&>;
}

///////////////////////////////////////////////////////////////////////////////
// CYCFI_FORCE_INLINE utility
///////////////////////////////////////////////////////////////////////////////
#ifdef _MSC_VER
   #define CYCFI_FORCE_INLINE __forceinline
#elif defined(__GNUC__) || defined(__clang__)
   #define CYCFI_FORCE_INLINE inline __attribute__((always_inline))
#else
   #define CYCFI_FORCE_INLINE inline
#endif

#endif
