/*=============================================================================
   Copyright (c) 2014-2024 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_DELAY_JULY_20_2014)
#define CYCFI_Q_DELAY_JULY_20_2014

#include <q/utility/fractional_ring_buffer.hpp>
#include <q/support/base.hpp>

namespace cycfi::q
{
   ////////////////////////////////////////////////////////////////////////////
   // Basic one unit delay
   ////////////////////////////////////////////////////////////////////////////
   struct delay1
   {
      float operator()(float s)
      {
         auto r = y;
         y = s;
         return r;
      }

      float operator()() const
      {
         return y;
      }

      void reset()
      {
         y = 0.0f;
      }

      float y = 0.0f;
   };

   ////////////////////////////////////////////////////////////////////////////
   // Basic two unit delay
   ////////////////////////////////////////////////////////////////////////////
   struct delay2
   {
      float operator()(float s)
      {
         return _d2(_d1(s));
      }

      float operator()() const
      {
         return _d2();
      }

      void reset()
      {
         _d1.reset();
         _d2.reset();
      }

      delay1 _d1, _d2;
   };

   ////////////////////////////////////////////////////////////////////////////
   // basic_delay: a basic class for delays. The actual delay parameter is
   // decoupled from, and managed outside the class to allow both single and
   // multi-tapped delays.
   //
   // delay and nf_delay type aliases are provided for fractional delays and
   // simpler non-fractional delays, respectively.
   ////////////////////////////////////////////////////////////////////////////
   template <typename Base>
   class basic_delay : public Base
   {
   public:

      using base_type = Base;
      using value_type = typename Base::value_type;
      using storage_type = typename Base::storage_type;
      using index_type = typename Base::index_type;
      using interpolation_type = typename Base::interpolation_type;

      basic_delay(duration max_delay, float sps)
       : base_type(std::size_t(std::ceil(as_double(max_delay) * sps)))
      {}

      explicit basic_delay(std::size_t max_delay_samples)
       : base_type(std::size_t(max_delay_samples))
      {}

      // Get the delayed signal (maximum delay).
      float operator()() const
      {
         return this->back();
      }

      // Get the delayed signal.
      float operator()(index_type i) const
      {
         return (*this)[i];
      }

      // Push a new signal and return the delayed signal at index `i+1`. This
      // is the simplest (common) case for single delays. For multi-tapped
      // delays, you need to access the individual delays using the indexing
      // operator for various tap-points before pushing the latest sample.
      float operator()(value_type val, index_type i)
      {
         float delayed = (*this)[i];
         this->push(val);
         return delayed;
      }
   };

   // Fractional delay
   using delay = basic_delay<fractional_ring_buffer<float>>;

   // Non-fractional delay
   using nf_delay = basic_delay<ring_buffer<float>>;

}

#endif
