/*=============================================================================
   Copyright (c) 2022 Joel de Guzman

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(INFRA_DATA_STREAM_JUNE_13_2019)
#define INFRA_DATA_STREAM_JUNE_13_2019

// This header has a dependency on msgpack, a header-only utility.
// Make sure the to add its include directory in your build settings.

#include <msgpack.hpp>
#include <infra/iterator_range.hpp>

namespace cycfi
{
   template <typename Derived>
   struct ostream
   {
      Derived& derived() { return *static_cast<Derived*>(this); }

      template <typename T>
      Derived& operator<<(T const& val)
      {
         msgpack::pack(derived(), val);
         return derived();
      }
   };

   template <typename Derived>
   struct istream
   {
      Derived& derived() { return *static_cast<Derived*>(this); }

      template <typename T>
      Derived& operator>>(T& val)
      {
         auto oh = msgpack::unpack(derived().data(), derived().size(), _offset);
         auto obj = oh.get();
         val = obj.template as<T>();
         return derived();
      }

      std::size_t offset() const { return _offset; }

   private:

      std::size_t _offset = 0;
   };
}

#endif