/*=============================================================================
   Copyright (c) 2022 Joel de Guzman

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(INFRA_STRIDE_ITERATOR_SEPTEMBER_14_2022)
#define INFRA_STRIDE_ITERATOR_SEPTEMBER_14_2022

#include <cstddef>
#include <iterator>

namespace cycfi
{
   ////////////////////////////////////////////////////////////////////////////
   // stride_iterator: An iterator that iterates in strides
   // (multiple elements per step)
   ////////////////////////////////////////////////////////////////////////////
   template <typename T>
   struct stride_iterator
   {
      using iterator_category = std::random_access_iterator_tag;
      using value_type = T;
      using difference_type = ptrdiff_t;
      using pointer = T*;
      using reference = T&;

      constexpr explicit            stride_iterator(
                                       T* rhs = nullptr,
                                       difference_type stride = difference_type{1}
                                    );
      constexpr                     stride_iterator(stride_iterator const&) = default;
      constexpr                     stride_iterator(stride_iterator&&) = default;

      constexpr stride_iterator&    operator=(stride_iterator const&) = default;
      constexpr stride_iterator&    operator=(stride_iterator&&) = default;

      constexpr stride_iterator&    operator+=(difference_type rhs);
      constexpr stride_iterator&    operator-=(difference_type rhs);
      constexpr T&                  operator*() const;
      constexpr T*                  operator->() const;
      constexpr T&                  operator[](difference_type rhs) const;

      constexpr stride_iterator&    operator++();
      constexpr stride_iterator&    operator--();
      constexpr stride_iterator     operator++(int);
      constexpr stride_iterator     operator--(int);

      constexpr T*                  base() const;

      T* _ptr;
      difference_type _stride;
   };

   ////////////////////////////////////////////////////////////////////////////
   // Free Functions API
   ////////////////////////////////////////////////////////////////////////////
   template <typename T>
   constexpr stride_iterator<T>
   operator+(stride_iterator<T> const& lhs, typename stride_iterator<T>::difference_type rhs);

   template <typename T>
   constexpr stride_iterator<T>
   operator-(stride_iterator<T> const& lhs, typename stride_iterator<T>::difference_type rhs);

   template <typename T>
   constexpr stride_iterator<T>
   operator+(typename stride_iterator<T>::difference_type lhs, stride_iterator<T> const& rhs);

   template <typename T>
   constexpr stride_iterator<T>
   operator-(typename stride_iterator<T>::difference_type lhs, stride_iterator<T> const& rhs);

   template <typename T>
   constexpr bool operator==(stride_iterator<T> const& lhs, stride_iterator<T> const& rhs);

   template <typename T>
   constexpr bool operator!=(stride_iterator<T> const& lhs, stride_iterator<T> const& rhs);

   template <typename T>
   constexpr bool operator>(stride_iterator<T> const& lhs, stride_iterator<T> const& rhs);

   template <typename T>
   constexpr bool operator<(stride_iterator<T> const& lhs, stride_iterator<T> const& rhs);

   template <typename T>
   constexpr bool operator>=(stride_iterator<T> const& lhs, stride_iterator<T> const& rhs);

   template <typename T>
   constexpr bool operator<=(stride_iterator<T> const& lhs, stride_iterator<T> const& rhs);

   ////////////////////////////////////////////////////////////////////////////
   // Implementation
   ////////////////////////////////////////////////////////////////////////////
   template <typename T>
   constexpr stride_iterator<T>::stride_iterator(T* rhs, difference_type stride)
         : _ptr{rhs}, _stride{stride}
   {}

   template <typename T>
   constexpr stride_iterator<T>& stride_iterator<T>::operator+=(difference_type rhs)
   {
      _ptr += rhs * _stride;
      return *this;
   }

   template <typename T>
   constexpr stride_iterator<T>& stride_iterator<T>::operator-=(difference_type rhs)
   {
      _ptr -= rhs * _stride;
      return *this;
   }

   template <typename T>
   constexpr T& stride_iterator<T>::operator*() const
   {
      return *_ptr;
   }

   template <typename T>
   constexpr T* stride_iterator<T>::operator->() const
   {
      return _ptr;
   }

   template <typename T>
   constexpr T* stride_iterator<T>::base() const
   {
      return _ptr;
   }

   template <typename T>
   constexpr T& stride_iterator<T>::operator[](difference_type rhs) const
   {
      return _ptr[rhs * _stride];
   }

   template <typename T>
   constexpr stride_iterator<T>& stride_iterator<T>::operator++()
   {
      _ptr += _stride;
      return *this;
   }

   template <typename T>
   constexpr stride_iterator<T>& stride_iterator<T>::operator--()
   {
      _ptr -= _stride;
      return *this;
   }

   template <typename T>
   constexpr stride_iterator<T> stride_iterator<T>::operator++(int)
   {
      stride_iterator tmp = *this;
      ++*this;
      return tmp;
   }

   template <typename T>
   constexpr stride_iterator<T> stride_iterator<T>::operator--(int)
   {
      stride_iterator tmp = *this;
      --*this;
      return tmp;
   }

   template <typename T>
   constexpr stride_iterator<T>
   operator+(stride_iterator<T> const& lhs, typename stride_iterator<T>::difference_type rhs)
   {
      return stride_iterator{lhs._ptr + (rhs * lhs._stride), lhs._stride};
   }

   template <typename T>
   constexpr stride_iterator<T>
   operator-(stride_iterator<T> const& lhs, typename stride_iterator<T>::difference_type rhs)
   {
      return stride_iterator{lhs._ptr - (rhs * lhs._stride), lhs._stride};
   }

   template <typename T>
   constexpr stride_iterator<T>
   operator+(typename stride_iterator<T>::difference_type lhs, stride_iterator<T> const& rhs)
   {
      return stride_iterator{(lhs * lhs._stride) + rhs._ptr, lhs._stride};
   }

   template <typename T>
   constexpr stride_iterator<T>
   operator-(typename stride_iterator<T>::difference_type lhs, stride_iterator<T> const& rhs)
   {
      return stride_iterator{(lhs * lhs._stride) - rhs._ptr, lhs._stride};
   }

   template <typename T>
   constexpr bool operator==(stride_iterator<T> const& lhs, stride_iterator<T> const& rhs)
   {
      return lhs._ptr == rhs._ptr;
   }

   template <typename T>
   constexpr bool operator!=(stride_iterator<T> const& lhs, stride_iterator<T> const& rhs)
   {
      return lhs._ptr != rhs._ptr;
   }

   template <typename T>
   constexpr bool operator>(stride_iterator<T> const& lhs, stride_iterator<T> const& rhs)
   {
      return lhs._ptr > rhs._ptr;
   }

   template <typename T>
   constexpr bool operator<(stride_iterator<T> const& lhs, stride_iterator<T> const& rhs)
   {
      return lhs._ptr < rhs._ptr;
   }

   template <typename T>
   constexpr bool operator>=(stride_iterator<T> const& lhs, stride_iterator<T> const& rhs)
   {
      return lhs._ptr >= rhs._ptr;
   }

   template <typename T>
   constexpr bool operator<=(stride_iterator<T> const& lhs, stride_iterator<T> const& rhs)
   {
      return lhs._ptr <= rhs._ptr;
   }
}

#endif
