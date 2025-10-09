/*=============================================================================
   Copyright (c) 2014-2024 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#include <q/support/literals.hpp>
#include <q/support/pitch_names.hpp>
#include <q/synth/square_osc.hpp>
#include <q_io/audio_file.hpp>
#include <array>

namespace q = cycfi::q;
using namespace q::literals;
using namespace q::pitch_names;

constexpr auto sps = 48000;

int main()
{
   ////////////////////////////////////////////////////////////////////////////
   // Synthesize a 10-second square wave

   constexpr auto size = sps * 10;
   constexpr auto n_channels = 1;
   constexpr auto buffer_size = size * n_channels;

   auto buff = std::array<float, buffer_size>{};   // The output buffer
   const auto f = q::phase(C[3], sps);             // The synth frequency
   auto ph = q::phase();                           // Our phase accumulator

   auto square = q::basic_square;                  // Our square synth

   for (auto i = 0; i != size; ++i)
   {
      buff[i] = square(ph) * 0.9;
      ph += f;
   }

   ////////////////////////////////////////////////////////////////////////////
   // Write to a wav file

   q::wav_writer wav(
      "results/synth_basic_square.wav", n_channels, sps // mono, 48000 sps
   );
   wav.write(buff);

   return 0;
}
