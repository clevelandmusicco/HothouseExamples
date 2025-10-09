/*=============================================================================
   Copyright (c) 2016-2023 Joel de Guzman
   Copyright (c) 2022      Jean Pierre Cimalando

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(INFRA_STRING_VIEW_MAY_22_2020)
#define INFRA_STRING_VIEW_MAY_22_2020

#if (__cplusplus >= 201703L) && (defined(__cpp_lib_string_view) || \
    (defined(__has_include) && __has_include(<string_view>)))
# define INFRA_USE_STD_STRING_VIEW
#endif

#if defined(INFRA_FORCE_STD_STRING_VIEW) && defined(INFRA_FORCE_NONSTD_STRING_VIEW)
# error two string view implementations should not be forced at the same time
#endif

#if defined(INFRA_FORCE_STD_STRING_VIEW)
# if !defined(INFRA_USE_STD_STRING_VIEW)
#  define INFRA_USE_STD_STRING_VIEW
# endif
#elif defined(INFRA_FORCE_NONSTD_STRING_VIEW)
# if defined(INFRA_USE_STD_STRING_VIEW)
#  undef INFRA_USE_STD_STRING_VIEW
# endif
#endif

#if defined(INFRA_USE_STD_STRING_VIEW)

#include <string_view>

namespace cycfi {
  using std::basic_string_view;
  using std::string_view;
  using std::wstring_view;
  using std::u16string_view;
  using std::u32string_view;
}

#else

#include <nonstd/string_view.hpp>

namespace cycfi {
  using nonstd::basic_string_view;
  using nonstd::string_view;
  using nonstd::wstring_view;
  using nonstd::u16string_view;
  using nonstd::u32string_view;
}

#endif

#endif
