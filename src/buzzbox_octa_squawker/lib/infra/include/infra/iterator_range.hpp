/*=============================================================================
   Copyright (c) 2016-2023 Joel de Guzman

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(INFRA_ITERATOR_RANGE_OCTOBER_15_2018)
#define INFRA_ITERATOR_RANGE_OCTOBER_15_2018

#include <cstddef>
#include <iterator>
#include <type_traits>
#include <infra/assert.hpp>

namespace cycfi
{
   ////////////////////////////////////////////////////////////////////////////
   // Iterator Range holds a random access iterator pair.
   ////////////////////////////////////////////////////////////////////////////
   template <typename Iterator>
   class iterator_range
   {
   public:

      using traits = std::iterator_traits<Iterator>;
      using difference_type = typename traits::difference_type;
      using value_type = typename traits::value_type;
      using pointer = typename traits::pointer;
      using reference = typename traits::reference;
      using iterator = Iterator;

      static_assert(
         std::is_same<
            typename traits::iterator_category
          , std::random_access_iterator_tag
         >::value
       , "Random access iterator required"
      );

                           iterator_range();
                           iterator_range(Iterator f, Iterator l);

                           template <std::size_t N>
                           iterator_range(value_type(&arr)[N]);

      std::size_t          size() const;
      Iterator             begin() const  { return _f; }
      Iterator             end() const    { return _l; }
      auto&                operator[](std::size_t i) const;

   private:

      Iterator             _f;
      Iterator             _l;
   };

   template <typename Iterator>
   iterator_range<Iterator>
   make_iterator_range(Iterator f, Iterator l);

   template <typename Container>
   inline auto make_iterator_range(Container&& c)
   {
      return make_iterator_range(
         std::begin(std::forward<Container>(c))
       , std::end(std::forward<Container>(c)));
   }

   ////////////////////////////////////////////////////////////////////////////
   // Implementation
   ////////////////////////////////////////////////////////////////////////////
   template <typename Iterator>
   iterator_range<Iterator>::iterator_range()
    : _f(), _l() {}

   template <typename Iterator>
   iterator_range<Iterator>::iterator_range(Iterator f, Iterator l)
    : _f(f), _l(l)
   {
      CYCFI_ASSERT((_f <= _l), "Invalid range");
   }

   template <typename Iterator>
   template <std::size_t N>
   iterator_range<Iterator>::iterator_range(value_type(&arr)[N])
    : _f(arr), _l(arr + N)
   {
   }

   template <typename Iterator>
   inline iterator_range<Iterator>
   make_iterator_range(Iterator f, Iterator l)
   {
      return iterator_range<Iterator>(f, l);
   }

   template <typename Iterator>
   inline auto&
   iterator_range<Iterator>::operator[](std::size_t i) const
   {
      CYCFI_ASSERT(i < size(), "Index out of range");
      return _f[i];
   }

   template <typename Iterator>
   inline std::size_t
   iterator_range<Iterator>::size() const
   {
      return _l - _f;
   }
}

#endif
