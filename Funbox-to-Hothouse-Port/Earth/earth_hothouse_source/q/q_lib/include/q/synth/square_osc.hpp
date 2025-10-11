/*=============================================================================
   Copyright (c) 2014-2024 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_SQUARE_OSC_HPP_DECEMBER_24_2015)
#define CYCFI_Q_SQUARE_OSC_HPP_DECEMBER_24_2015

#include <q/support/phase.hpp>
#include <q/utility/antialiasing.hpp>

namespace cycfi::q
{
   ////////////////////////////////////////////////////////////////////////////
   // basic square-wave oscillator (not bandwidth limited)
   ////////////////////////////////////////////////////////////////////////////
   struct basic_square_osc
   {
      constexpr float operator()(phase p) const
      {
         constexpr auto middle = phase::middle();
         return p < middle ? 1.0f : -1.0f;
      }

      constexpr float operator()(phase_iterator i) const
      {
         return (*this)(i._phase);
      }
   };

   constexpr auto basic_square = basic_square_osc{};

   ////////////////////////////////////////////////////////////////////////////
   // square-wave oscillator (bandwidth limited using poly_blep)
   ////////////////////////////////////////////////////////////////////////////
   struct square_osc
   {
      constexpr float operator()(phase p, phase dt) const
      {
         constexpr auto middle = phase::middle();
         auto r = p < middle ? 1.0f : -1.0f;

         // Correct rising discontinuity
         r += poly_blep(p, dt);

         // Correct falling discontinuity
         r -= poly_blep(p + middle, dt);

         return r;
      }

      constexpr float operator()(phase_iterator i) const
      {
         return (*this)(i._phase, i._step);
      }
   };

   constexpr auto square = square_osc{};
}

#endif
