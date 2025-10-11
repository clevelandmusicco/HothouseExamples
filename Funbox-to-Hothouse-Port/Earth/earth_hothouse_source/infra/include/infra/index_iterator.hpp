/*=============================================================================
   Copyright (c) 2022 Joel de Guzman

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(INFRA_INDEX_ITERATOR_MAY_28_2023)
#define INFRA_INDEX_ITERATOR_MAY_28_2023

#include <cstddef>

namespace cycfi
{
   ////////////////////////////////////////////////////////////////////////////
   // index_iterator: An iterator that iterates over over a std::size_t index.
   ////////////////////////////////////////////////////////////////////////////
   struct index_iterator
   {
      using iterator_category = std::random_access_iterator_tag;
      using difference_type = std::ptrdiff_t;
      using value_type = std::size_t;
      using pointer = std::size_t*;
      using reference = std::size_t&;

      constexpr operator std::size_t() const     { return i; }
      constexpr operator std::size_t&()          { return i; }
      constexpr std::size_t operator*() const    { return i; }

      std::size_t i;
   };
}

#endif
