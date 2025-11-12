/*=============================================================================
   Copyright (c) 2016-2023 Joel de Guzman
   Copyright (c) 2022      Jean Pierre Cimalando

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(INFRA_OPTIONAL_JUNE_14_2020)
#define INFRA_OPTIONAL_JUNE_14_2020

#if (__cplusplus >= 201703L) && (defined(__cpp_lib_optional) || \
    (defined(__has_include) && __has_include(<optional>)))
# define INFRA_USE_STD_OPTIONAL
#endif

#if defined(INFRA_FORCE_STD_OPTIONAL) && defined(INFRA_FORCE_NONSTD_OPTIONAL)
# error two optional implementations should not be forced at the same time
#endif

#if defined(INFRA_FORCE_STD_OPTIONAL)
# if !defined(INFRA_USE_STD_OPTIONAL)
#  define INFRA_USE_STD_OPTIONAL
# endif
#elif defined(INFRA_FORCE_NONSTD_OPTIONAL)
# if defined(INFRA_USE_STD_OPTIONAL)
#  undef INFRA_USE_STD_OPTIONAL
# endif
#endif

#if defined(INFRA_USE_STD_OPTIONAL)

#include <optional>

namespace cycfi {
  using std::optional;
  using std::make_optional;
  using std::nullopt_t;
  using std::nullopt;
  using std::bad_optional_access;
}

#else

#include <nonstd/optional.hpp>

namespace cycfi {
  using nonstd::optional;
  using nonstd::make_optional;
  using nonstd::nullopt_t;
  using nonstd::nullopt;
  using nonstd::bad_optional_access;
}

#endif

#endif
