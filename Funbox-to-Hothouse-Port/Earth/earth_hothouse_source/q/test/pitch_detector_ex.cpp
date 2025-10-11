/*=============================================================================
   Copyright (c) 2014-2024 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#define CATCH_CONFIG_MAIN
#include <infra/catch.hpp>

#include <q/support/literals.hpp>
#include <q/pitch/pitch_detector.hpp>
#include <q_io/audio_file.hpp>
#include <q/fx/envelope.hpp>
#include <q/fx/lowpass.hpp>
#include <q/fx/biquad.hpp>
#include <q/fx/dynamic.hpp>
#include <q/fx/clip.hpp>

#include <vector>
#include <iostream>
#include <fstream>
#include <chrono>

#include "pitch.hpp"

namespace q = cycfi::q;
using namespace q::literals;

int get_num(std::string const& s, int pos, float& num)
{
   auto new_pos = s.find_first_of(", ", pos);
   num = std::stod(s.substr(pos, new_pos));
   return new_pos + 2;
}

constexpr bool skip_tests = true;
constexpr auto break_time = 100.0;

void break_debug() // seconds
{
   // Set break_time to a specific time if you want to do some
   // timed debugging. Then set a break-point here.
}

void compare_golden(std::string name)
{
   std::ifstream a("results/frequencies_" + name + ".csv");
   std::ifstream b("golden/frequencies_" + name + ".csv");

   CHECK(a);
   CHECK(b);

   std::string a_line, b_line;
   int index = 0;
   while (std::getline(a, a_line))
   {
      REQUIRE(std::getline(b, b_line));
      float fa, fb, pa, pb, ta, tb;

      auto posa = get_num(a_line, 0, fa);
      auto posb = get_num(b_line, 0, fb);

      posa = get_num(a_line, posa, pa);
      posb = get_num(b_line, posb, pb);

      get_num(a_line, posa, ta);
      get_num(b_line, posb, tb);

      INFO(
         "In test: \""
         << name
         << "\", line: "
         << index
      );

      REQUIRE(fa == Approx(fb));
      REQUIRE(pa == Approx(pb));
      REQUIRE(ta == Approx(tb));
      ++index;
   }
}

void process(
   std::string name
 , q::frequency lowest_freq
 , q::frequency highest_freq)
{
   ////////////////////////////////////////////////////////////////////////////
   // Prepare output file

   std::ofstream csv("results/frequencies_" + name + ".csv");

   ////////////////////////////////////////////////////////////////////////////
   // Read audio file

   q::wav_reader src{"audio_files/" + name + ".wav"};
   float const sps = src.sps();

   std::vector<float> in(src.length());
   src.read(in);

   ////////////////////////////////////////////////////////////////////////////
   // Output
   constexpr auto n_channels = 5;
   std::vector<float> out(src.length() * n_channels);
   std::fill(out.begin(), out.end(), 0);

   ////////////////////////////////////////////////////////////////////////////
   // Process
   q::pitch_detector          pd{ lowest_freq, highest_freq, sps, -40_dB };
   auto const&                bits = pd.bits();
   auto const&                edges = pd.edges();
   q::bitstream_acf<>         bacf{ bits };
   auto                       min_period = as_float(highest_freq.period()) * sps;

   q::peak_envelope_follower  env{ 30_ms, sps };
   q::one_pole_lowpass        lp{ highest_freq, sps };
   q::one_pole_lowpass        lp2{ lowest_freq, sps };

   constexpr float            slope = 1.0f/4;
   constexpr float            makeup_gain = 4;
   q::compressor              comp{ -18_dB, slope };
   q::clip                    clip;

   float                      onset_threshold = lin_float(-28_dB);
   float                      release_threshold = lin_float(-60_dB);
   float                      threshold = onset_threshold;

   std::uint64_t              nanoseconds = 0;

   for (auto i = 0; i != in.size(); ++i)
   {
      auto pos = i * n_channels;
      auto ch1 = pos;      // input
      auto ch2 = pos+1;    // zero crossings
      auto ch3 = pos+2;    // bacf
      auto ch4 = pos+3;    // frequency
      auto ch5 = pos+4;    // predicted frequency

      float time = i / float(sps);

      auto s = in[i];

      // Bandpass filter
      s = lp(s);
      s -= lp2(s);

      // Envelope
      auto e = env(std::abs(s));
      auto e_db = q::lin_to_db(e);

      if (e > threshold)
      {
         // Compressor + makeup-gain + hard clip
         auto gain = lin_float(comp(e_db)) * makeup_gain;
         s = clip(s * gain);
         threshold = release_threshold;
      }
      else
      {
         s = 0.0f;
         threshold = onset_threshold;
      }

      out[ch1] = s;

      if (time >= break_time)
         break_debug();

      // Pitch Detect
      auto start = std::chrono::high_resolution_clock::now();
      bool ready = pd(s);
      auto elapsed = std::chrono::high_resolution_clock::now() - start;
      auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(elapsed);
      nanoseconds += duration.count();

      out[ch2] = -0.8;  // placeholder for bitset bits
      out[ch3] = 0.0f;  // placeholder for autocorrelation results
      out[ch4] = -0.8;  // placeholder for frequency

      if (ready)
      {
         auto frame = edges.frame() + (edges.window_size() / 2);
         auto extra = frame - edges.window_size();
         auto size = bits.size();

         // Print the bitset bits
         {
            auto out_i = (&out[ch2] - (((size-1) + extra) * n_channels));
            for (auto i = 0; i != size; ++i)
            {
               *out_i = bits.get(i) * 0.8;
               out_i += n_channels;
            }
         }

         // Print the autocorrelation results
         {
            auto weight = 2.0 / size;
            auto out_i = (&out[ch3] - (((size-1) + extra) * n_channels));
            for (auto i = 0; i != size/2; ++i)
            {
               if (i > min_period)
                  *out_i = 1.0f - (bacf(i) * weight);
               out_i += n_channels;
            }
         }

         {
            auto f = pd.get_frequency();
            auto p = pd.periodicity();
            auto f2 = float(sps) / pd.get_period_detector().fundamental()._period;
            auto fr = pd.frames_after_shift();
            csv << f << ", " << f2 << ", " << p << ", " << fr << ", " << time << std::endl;
         }

         // Print the frequency
         {
            auto f = pd.get_frequency() / as_double(highest_freq);
            auto out_i = (&out[ch4] - (((size-1) + extra) * n_channels));
            for (auto i = 0; i != size; ++i)
            {
               *out_i = f;
               out_i += n_channels;
            }
         }
      }

      // Print the predicted frequency
      {
         auto f = pd.predict_frequency() / as_double(highest_freq);
         out[ch5] = f;
      }
   }

   csv.close();

   ////////////////////////////////////////////////////////////////////////////
   // Print processing time
   std::cout
      << '"' << name << "\": "
      << (double(nanoseconds) / in.size())
      << " nanoseconds per sample."
      << std::endl
      ;

   ////////////////////////////////////////////////////////////////////////////
   // Compare to golden
   if (!skip_tests)
      compare_golden(name);

   ////////////////////////////////////////////////////////////////////////////
   // Write to a wav file

   q::wav_writer wav(
      "results/pitch_detect_" + name + ".wav", n_channels, sps
   );
   wav.write(out);
}

void process(std::string name, q::frequency lowest_freq)
{
   process(name, lowest_freq * 0.8, lowest_freq * 5);
}

using namespace notes;

TEST_CASE("Test_low_frequencies")
{
   process("-2a-F#", low_fs);
   process("-2b-F#-12th", low_fs);
   process("-2c-F#-24th", low_fs);

   process("-1a-Low-B", low_b);
   process("-1b-Low-B-12th", low_b);
   process("-1c-Low-B-24th", low_b);
}

TEST_CASE("Test_basic")
{
   process("sin_440", d);

   process("1a-Low-E", low_e);
   process("1b-Low-E-12th", low_e);
   process("1c-Low-E-24th", low_e);

   process("2a-A", a);
   process("2b-A-12th", a);
   process("2c-A-24th", a);

   process("3a-D", d);
   process("3b-D-12th", d);
   process("3c-D-24th", d);

   process("4a-G", g);
   process("4b-G-12th", g);
   process("4c-G-24th", g);

   process("5a-B", b);
   process("5b-B-12th", b);
   process("5c-B-24th", b);

   process("6a-High-E", high_e);
   process("6b-High-E-12th", high_e);
   process("6c-High-E-24th", high_e);
}

TEST_CASE("Test_phrase")
{
   process("Tapping D", d);
   process("Hammer-Pull High E", high_e);
   process("Slide G", g);
   process("Bend-Slide G", g);
}

TEST_CASE("Test_staccato")
{
   process("GLines1", g);
   process("GLines2", g);
   process("GLines3", g);
   process("SingleStaccato", g);
   process("GStaccato", g);
   process("ShortStaccato", g);
   process("Attack-Reset", g);
}

