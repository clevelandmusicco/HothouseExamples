/*=============================================================================
   Copyright (c) 2016-2023 Joel de Guzman

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_ASSERT_OCTOBER_4_2016)
#define CYCFI_ASSERT_OCTOBER_4_2016

#include <cassert>

#define CYCFI_ASSERT(x, msg) assert(((x) && msg)); (void)(x), void(msg)

#endif
