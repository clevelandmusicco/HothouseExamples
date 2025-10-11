/*=============================================================================
   Copyright (c) 2014-2024 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_SAW_OSC_HPP_DECEMBER_24_2015)
#define CYCFI_Q_SAW_OSC_HPP_DECEMBER_24_2015

#include <q/support/phase.hpp>
#include <q/utility/antialiasing.hpp>

namespace cycfi::q
{
   ////////////////////////////////////////////////////////////////////////////
   // basic sawtooth-wave oscillator (not bandwidth limited)
   ////////////////////////////////////////////////////////////////////////////
   struct basic_saw_osc
   {
      constexpr float operator()(phase p) const
      {
         constexpr float x = 2.0f / phase::one_cyc;
         return (p.rep * x) - 1.0;
      }

      constexpr float operator()(phase_iterator i) const
      {
         return (*this)(i._phase);
      }
   };

   constexpr auto basic_saw = basic_saw_osc{};

   ////////////////////////////////////////////////////////////////////////////
   // sawtooth-wave oscillator (bandwidth limited)
   ////////////////////////////////////////////////////////////////////////////
   struct saw_osc
   {
      constexpr float operator()(phase p, phase dt) const
      {
         constexpr float x = 2.0f / phase::one_cyc;
         auto r = (p.rep * x) - 1.0;

         // Correct discontinuity
         r -= poly_blep(p, dt);
         return r;
      }

      constexpr float operator()(phase_iterator i) const
      {
         return (*this)(i._phase, i._step);
      }
   };

   constexpr auto saw = saw_osc{};
}

#endif
