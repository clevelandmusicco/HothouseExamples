/*=============================================================================
   Copyright (c) 2014-2024 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_TRIANGLE_OSC_HPP_DECEMBER_24_2015)
#define CYCFI_Q_TRIANGLE_OSC_HPP_DECEMBER_24_2015

#include <q/support/phase.hpp>
#include <q/utility/antialiasing.hpp>

namespace cycfi::q
{
  ////////////////////////////////////////////////////////////////////////////
   // basic triangle-wave oscillator (not bandwidth limited)
   ////////////////////////////////////////////////////////////////////////////
   struct basic_triangle_osc
   {
      constexpr float operator()(phase p) const
      {
         constexpr float x = 4.0f / phase::one_cyc;
         return (abs(std::int32_t(p.rep)) * x) - 1.0;
      }

      constexpr float operator()(phase_iterator i) const
      {
         return (*this)(i._phase);
      }
   };

   constexpr auto basic_triangle = basic_triangle_osc{};

   ////////////////////////////////////////////////////////////////////////////
   // triangle-wave oscillator (bandwidth limited)
   ////////////////////////////////////////////////////////////////////////////
   struct triangle_osc
   {
      constexpr float operator()(phase p, phase dt) const
      {
         constexpr auto end = phase::end();
         constexpr auto edge1 = end/4;
         constexpr auto edge2 = end-edge1;
         constexpr float x = 4.0f / phase::one_cyc;

         auto r = (abs(std::int32_t((p + edge1).rep)) * x) - 1.0;

         // Correct falling discontinuity
         r += poly_blamp(p + edge1, dt, 4);

         // Correct rising discontinuity
         r -= poly_blamp(p + edge2, dt, 4);

         return r;
      }

      constexpr float operator()(phase_iterator i) const
      {
         return (*this)(i._phase, i._step);
      }
   };

   constexpr auto triangle = triangle_osc{};
}

#endif
