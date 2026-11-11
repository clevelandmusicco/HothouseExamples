// Indra for Hothouse DIY DSP Platform
// Copyright (C) 2024 Jon Philpott <jon.philpott@gmail.com>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

// Port of the fez9000 PureData patch AutomataAutoGene.pd.
//
// The big picture:
//   1. input runs through a 250ms pre-delay and, when something loud
//      happens (or FS2 is tapped), the next 10s get recorded into a buffer.
//   2. seven "voices" loop that buffer forever at different speeds and
//      directions (octave down, unison, fifth up, octave up, some reversed).
//   3. an 11 cell cellular automaton steps once per clock tick. every cell
//      that's alive (re)opens the envelope on its voice, so the automaton
//      decides which voices you hear on each beat.
//
// Comments like "pd: [delread4~]" point at the object in the original patch
// that the code below replaces.

// ### Uncomment if IntelliSense can't resolve DaisySP-LGPL classes ###
// #include "daisysp-lgpl.h"

#include <algorithm>

#include "daisysp.h"
#include "hothouse.h"

using clevelandmusicco::Hothouse;
using daisy::AudioHandle;
using daisy::Led;
using daisy::SaiHandle;
using daisy::System;
using daisysp::Adsr;
using daisysp::CrossFade;
using daisysp::DelayLine;
using daisysp::fclamp;
using daisysp::fmap;
using daisysp::fonepole;
using daisysp::Limiter;
using daisysp::Mapping;
using daisysp::Maytrig;
using daisysp::Metro;
using daisysp::OnePole;

#define SAMPLE_RATE (48000)
#define BLOCK_SIZE (4)

// pd: [table $0-buff 441003] -- 10 seconds of audio. at 48k that's 480000
// floats = ~1.9MB, way too big for the internal RAM, so it lives in SDRAM.
#define BUFFER_LEN static_cast<size_t>(SAMPLE_RATE * 10)

// pd: [delwrite~ $0-dl 300] / [delread4~ $0-dl 250]
#define PREDELAY_LEN static_cast<size_t>(SAMPLE_RATE * 0.3f)
#define PREDELAY_TIME (SAMPLE_RATE * 0.25f)

// pd: SPIGOTTER [delay 10000] -- ignore the input for 10s so we don't keep
// restarting recordings. pd started the 10s at the trigger, so it ran out
// just as the recording finished and the next loud note could start
// another one straight away. here it starts when the recording *ends* (or
// at the trigger, if MAYBE said no).
#define LOCKOUT_SAMPLES static_cast<uint32_t>(SAMPLE_RATE * 10)

// pd: GRAIN-ENV [delay 10] / [delay 4000]
#define DIP_SAMPLES static_cast<uint32_t>(SAMPLE_RATE * 0.01f)
#define HOLD_SAMPLES static_cast<uint32_t>(SAMPLE_RATE * 4.0f)

// bypass fades the mix back to full dry over 1s. this is the step per
// sample.
#define FADE_STEP (1.0f / SAMPLE_RATE)

// pd scaled each bus by 0.5, which sat too far under the dry signal. 1.125
// sounded right with only the 4 left voices, but mono sums all 7, and that
// pushed the limiter into clipping. scaled down by sqrt(4/7) (~0.76) so 7
// voices are about as loud as those 4 were. (independent signals add in
// power, so loudness grows with the square root of the voice count.)
#define WET_BOOST (0.85f)

Hothouse hw;

float DSY_SDRAM_BSS BUFFER[BUFFER_LEN];
DelayLine<float, PREDELAY_LEN> DSY_SDRAM_BSS PREDELAY;

// ---------------------------------------------------------------------------
// Voices
// ---------------------------------------------------------------------------

enum { BUS_L, BUS_R };

// one looping playhead over the buffer plus its envelope.
//
// pd: [pd GRAIN-PLAYER] / [pd REV-GRAIN-PLAYER] + [pd GRAIN-ENV]
//
// why not daisysp::Phasor? the patch reads the buffer with a phasor~ running
// at 0.05-0.2 Hz. Phasor keeps its phase as a float in radians, and at those
// tiny increments float rounding makes the pitch wander by tens of cents.
// instead `pos` counts samples directly. the speeds (0.5, 1, 1.5, 2) are all
// exact binary fractions, so pos += speed never accumulates any error.
struct Voice {
  float speed;      // samples per sample, negative = reversed
  float pos;        // read position in samples, 0..BUFFER_LEN
  float threshold;  // intensity knob has to be above this for us to fire
  int cell;         // which automaton cell drives this voice
  int bus;          // BUS_L or BUS_R

  Adsr env;
  uint32_t dip;   // samples left in the 10ms "duck to zero" phase
  uint32_t hold;  // samples left before the envelope releases
  float window;   // half-sine window value, updated once per block

  void Init(float samplerate) {
    env.Init(samplerate);
    env.SetAttackTime(0.08f);
    env.SetDecayTime(0.01f);
    env.SetSustainLevel(1.0f);  // sustain at 1 so decay never matters
    env.SetReleaseTime(0.08f);
    dip = hold = 0;
    window = 0;
  }

  // pd: GRAIN-ENV [t b b b] -> "0 10", [delay 10] -> "1 80",
  //     [delay 4000] -> "0 80"
  //
  // the patch ducks to 0 over 10ms, ramps back up over 80ms, and lets go
  // 4s after the *last* trigger. since a live cell retriggers every tick,
  // the duck happens on every beat. that's the rhythmic chop.
  //
  // Adsr does the ramps. we drive its gate with two countdowns: gate low
  // during the dip (with a 10ms release so it ducks fast), then gate high
  // for the hold. the rising edge after the dip kicks off the 80ms attack.
  void Trigger() {
    dip = DIP_SAMPLES;
    hold = HOLD_SAMPLES;
    env.SetReleaseTime(0.01f);
  }

  // pd: [phasor~] -> [*~ 441000] -> [tabread4~] -> [*~ window] -> [*~ env]
  float Process() {
    // 1. work out the gate for this sample
    bool gate = false;
    if (dip > 0) {
      // back to the normal 80ms release once the dip is done, so the
      // final fade-out after the hold is the slower one.
      if (--dip == 0) env.SetReleaseTime(0.08f);
    } else if (hold > 0) {
      hold--;
      gate = true;
    }
    float amp = env.Process(gate);

    // 2. read the buffer with linear interpolation. pd used 4-point
    // (tabread4~), but our positions only ever land on whole or half
    // samples so linear is plenty.
    size_t i0 = static_cast<size_t>(pos);
    size_t i1 = (i0 + 1 >= BUFFER_LEN) ? 0 : i0 + 1;
    float frac = pos - i0;
    float s = BUFFER[i0] + frac * (BUFFER[i1] - BUFFER[i0]);

    // 3. move the playhead and wrap it in both directions
    pos += speed;
    if (pos >= BUFFER_LEN) pos -= BUFFER_LEN;
    if (pos < 0) pos += BUFFER_LEN;

    return s * window * amp;
  }
};

// straight out of the patch. see the plan/README for the table.
//   speed, start pos, threshold, cell, bus
Voice VOICES[] = {
    {1.0f, 0.0f, 0.05f, 9, BUS_L},     // A: unison
    {0.5f, 0.1f, 0.25f, 8, BUS_R},     // B: octave down
    {1.5f, 0.33f, 0.5f, 7, BUS_L},     // C: fifth up
    {2.0f, 0.75f, 0.7f, 6, BUS_R},     // D: octave up
    {-1.0f, 0.66f, 0.33f, 5, BUS_L},   // E: unison, reversed
    {-0.5f, 0.5f, 0.6f, 4, BUS_R},     // F: octave down, reversed
    {-2.0f, 0.125f, 0.75f, 3, BUS_L},  // G: octave up, reversed
};
#define N_VOICES (sizeof(VOICES) / sizeof(VOICES[0]))

// ---------------------------------------------------------------------------
// Cellular automaton
// ---------------------------------------------------------------------------
//
// pd: [FezAutomata 30]
//
// an "elementary" cellular automaton: a row of cells, each 0 or 1. on every
// step each cell looks at itself and its two neighbours. that's 3 bits, so
// 8 possible patterns. the rule number is an 8 bit lookup table: bit N of
// the rule is the cell's next value when the pattern spells N in binary
// (left*4 + centre*2 + right). rule 30 = 0b00011110, and so on.
//
// the patch uses 11 cells joined in a ring (cell 10's left neighbour is
// cell 0). we keep them as bits in a uint16_t.

#define N_CELLS (11)
#define SEED (1 << 5)  // pd: FezAutomata INIT sends 32 = just cell 5 alive

// the toggles pick a rule. T1 picks a family, T2/T3 pick one of 9 in it.
// all of these were simulated on this exact 11 cell ring from SEED and keep
// moving through all seven voices. index 0 of the middle family is rule 30
// which is what the patch used.
const uint8_t RULES[3][9] = {
    // T1 middle: chaotic, takes 110-979 steps to repeat
    {30, 86, 135, 149, 45, 75, 89, 101, 110},
    // T1 up: slower evolving and sparser, repeats every 110-341 steps
    {60, 102, 153, 195, 169, 225, 124, 137, 193},
    // T1 down: rhythmic, short audible loops of 8-88 steps
    {105, 150, 90, 165, 26, 154, 73, 57, 41},
};

uint16_t CELLS = SEED;

uint16_t AutomatonStep(uint16_t cells, uint8_t rule) {
  uint16_t next = 0;
  for (int i = 0; i < N_CELLS; i++) {
    // higher index = left, as wired in FezAutomata. the % wraps the ends.
    int l = (cells >> ((i + 1) % N_CELLS)) & 1;
    int c = (cells >> i) & 1;
    int r = (cells >> ((i + N_CELLS - 1) % N_CELLS)) & 1;
    next |= ((rule >> ((l << 2) | (c << 1) | r)) & 1) << i;
  }

  // changing rule mid-pattern carries the cells over, and some rule/state
  // combos die (all zero) or freeze (same state forever). either way the
  // music stops moving, so start again from the seed.
  if (next == 0 || next == cells) next = SEED;
  return next;
}

// toggle position -> 0/1/2 with middle as 0, so all-middle = RULES[0][0]
int TogglePos(Hothouse::Toggleswitch t) {
  switch (hw.GetToggleswitchPosition(t)) {
    case Hothouse::TOGGLESWITCH_UP:
      return 1;
    case Hothouse::TOGGLESWITCH_DOWN:
      return 2;
    default:
      return 0;
  }
}

// ---------------------------------------------------------------------------
// State shared between the audio callback and the main loop
// ---------------------------------------------------------------------------

Led led_enabled;
Led led_tempo;

// `volatile` because the audio interrupt writes these and the main loop
// reads them. it stops the compiler caching them in a register.
volatile bool BYPASS = true;
volatile bool RECORDING = false;
volatile bool CLEAR_REQUESTED = false;
volatile uint32_t TICK_COUNT = 0;

// the patch hard-pans its two buses, so with a mono cable you'd only hear
// the four left voices. default is mono (both buses summed to both jacks).
// hold FS1 while powering up for stereo.
bool STEREO = false;

size_t REC_POS = 0;
uint32_t LOCKOUT = 0;
float ENV_SQ = 0;    // smoothed input power for the trigger
float WET_FADE = 0;  // 0..1, ramps over 1s on bypass/engage

Metro CLOCK;
Maytrig MAYBE;
OnePole TONE[2];
Limiter LIMITERS[2];
CrossFade MIX;

// pd: [tabwrite~] bang. starts (or restarts) a 10s recording.
void StartRecording() {
  REC_POS = 0;
  RECORDING = true;
  // the countdown is frozen while recording (see step 4), so this really
  // means "10s after the recording finishes". forced recordings get it too.
  LOCKOUT = LOCKOUT_SAMPLES;
}

// footswitch callbacks run inside hw.ProcessAllControls(), so inside the
// audio callback. keep them tiny and just flip state.
void OnNormalPress(Hothouse::Switches fs) {
  // pd: "force" inlet -> [s $0-force] -- skips the probability and lockout
  if (fs == Hothouse::FOOTSWITCH_2) StartRecording();
}

void OnDoublePress(Hothouse::Switches fs) {
  // pd: "clear" inlet -> [; $0-buff const 0]
  // the first tap of the double tap already started a recording, so stop
  // it. zeroing 1.9MB takes far too long for a 4 sample callback, the main
  // loop does it.
  if (fs == Hothouse::FOOTSWITCH_2) {
    RECORDING = false;
    CLEAR_REQUESTED = true;
  }
}

Hothouse::FootswitchCallbacks FOOTSWITCH_CALLBACKS = {
    OnNormalPress, OnDoublePress, nullptr};

// pd: [r $0-clock] -> FezAutomata -> [sel 1] -> GRAIN-ENV -> [< int]
void OnClockTick(uint8_t rule, float intensity) {
  CELLS = AutomatonStep(CELLS, rule);
  for (size_t v = 0; v < N_VOICES; v++) {
    if (((CELLS >> VOICES[v].cell) & 1) && VOICES[v].threshold < intensity) {
      VOICES[v].Trigger();
    }
  }
  TICK_COUNT = TICK_COUNT + 1;
}

void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out,
                   size_t size) {
  hw.ProcessAllControls();

  // 1. controls
  BYPASS ^= hw.switches[Hothouse::FOOTSWITCH_1].RisingEdge();

  float bpm = fmap(hw.GetKnobValue(Hothouse::KNOB_1), 40.0f, 200.0f);
  float intensity = hw.GetKnobValue(Hothouse::KNOB_2);
  float probability = hw.GetKnobValue(Hothouse::KNOB_3);
  // pots and the ADC never quite hit 0.0 or 1.0, and the crossfade is
  // steep at the ends: a reading of 0.98 still leaks ~3% dry (-30dB). a 3%
  // dead zone at each end makes fully left/right exactly all dry/all wet.
  float mix_knob =
      fclamp((hw.GetKnobValue(Hothouse::KNOB_4) - 0.03f) / 0.94f, 0.0f, 1.0f);
  float threshold_db = fmap(hw.GetKnobValue(Hothouse::KNOB_5), -60.0f, -20.0f);
  float tone = fmap(hw.GetKnobValue(Hothouse::KNOB_6), 80.0f, 8000.0f,
                    Mapping::LOG);

  // the env follower tracks power (x squared), so compare against power
  // too: dB/10 instead of dB/20 saves a square root per sample.
  float threshold_sq = daisysp::pow10f(threshold_db / 10.0f);

  uint8_t rule = RULES[TogglePos(Hothouse::TOGGLESWITCH_1)]
                      [TogglePos(Hothouse::TOGGLESWITCH_2) * 3 +
                       TogglePos(Hothouse::TOGGLESWITCH_3)];

  CLOCK.SetFreq(bpm / 60.0f);  // quarter notes
  TONE[BUS_L].SetFrequency(tone / SAMPLE_RATE);
  TONE[BUS_R].SetFrequency(tone / SAMPLE_RATE);

  // 2. grain windows. pd: [-~ 0.5] -> [*~ 0.5] -> [cos~], which is just
  // sin(pi * phase): 0 at both ends of the buffer, 1 in the middle. one
  // window lasts 5-20s so working it out once per 4 sample block is
  // plenty, and sinf is the priciest thing in here.
  for (size_t v = 0; v < N_VOICES; v++) {
    VOICES[v].window = sinf(PI_F * VOICES[v].pos / BUFFER_LEN);
  }

  // dry/wet mix. bypass ramps the mix back to full dry over 1s (rather
  // than cutting the wet), so turning the effect off always lands on the
  // clean signal at unity whatever the knob says. the ramp moves once per
  // block, 4 samples' worth at a time, so the crossfade's two sinf calls
  // run once per block too.
  float wet_target = BYPASS ? 0.0f : 1.0f;
  WET_FADE += fclamp(wet_target - WET_FADE, -FADE_STEP * size,
                     FADE_STEP * size);
  MIX.SetPos(mix_knob * WET_FADE);

  for (size_t i = 0; i < size; ++i) {
    float dry = in[0][i];

    // 3. pre-delay. pd: [delwrite~] / [delread4~ 250]
    // the trigger hears the input now but we record what came 250ms ago,
    // so the note that tripped the trigger isn't missing its attack.
    PREDELAY.Write(dry);
    float delayed = PREDELAY.Read();

    // 4. auto-trigger. pd: [env~] -> [> 65] -> [spigot] -> [pd MAYBE]
    // fonepole is a one-pole smoother; ~23ms like env~'s 1024 window.
    fonepole(ENV_SQ, dry * dry, 0.0009f);
    if (RECORDING) {
      // no auto-triggers mid-recording, and the lockout waits for the end
    } else if (LOCKOUT > 0) {
      LOCKOUT--;
    } else if (!BYPASS && ENV_SQ > threshold_sq) {
      // lock out on every trigger, even when MAYBE says no. same as pd.
      LOCKOUT = LOCKOUT_SAMPLES;
      if (MAYBE.Process(probability)) StartRecording();
    }

    // 5. record. pd: [tabwrite~ $0-buff], stops when the table is full
    if (RECORDING) {
      BUFFER[REC_POS++] = delayed;
      if (REC_POS >= BUFFER_LEN) RECORDING = false;
    }

    // 6. clock
    if (CLOCK.Process()) OnClockTick(rule, intensity);

    // 7. voices onto their buses. pd: [throw~ $0-lout] / [throw~ $0-rout]
    float bus[2] = {0, 0};
    for (size_t v = 0; v < N_VOICES; v++) {
      bus[VOICES[v].bus] += VOICES[v].Process();
    }

    // 8. pd: [catch~] -> [*~ 0.5] -> [lop~ 800]
    // mono: fold the right bus into the left and copy it to both jacks.
    // only one tone filter gets used, the other just sits idle.
    if (!STEREO) {
      bus[BUS_L] += bus[BUS_R];
      bus[BUS_R] = 0;
    }
    float wet_l = TONE[BUS_L].Process(bus[BUS_L] * WET_BOOST);
    float wet_r = STEREO ? TONE[BUS_R].Process(bus[BUS_R] * WET_BOOST) : wet_l;

    // out you go. constant power crossfade: at noon both sides sit at
    // ~0.71 (-3dB) rather than 0.5, so the middle doesn't sound like a dip.
    out[0][i] = MIX.Process(dry, wet_l);
    out[1][i] = MIX.Process(dry, wet_r);
  }

  // 9. limiter on the final mix. daisysp's Limiter always multiplies by 0.7
  // before soft clipping, so the 1/0.7 pre-gain undoes that and quiet
  // signals come out at the same level as true bypass.
  LIMITERS[0].ProcessBlock(out[0], size, 1.0f / 0.7f);
  LIMITERS[1].ProcessBlock(out[1], size, 1.0f / 0.7f);
}

int main() {
  hw.Init();
  hw.SetAudioBlockSize(BLOCK_SIZE);  // Number of samples handled per callback
  hw.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_48KHZ);

  float samplerate = hw.AudioSampleRate();

  // SDRAM isn't zeroed at boot, so clear the buffer or the voices would
  // play whatever garbage was left in there.
  std::fill(BUFFER, BUFFER + BUFFER_LEN, 0.0f);

  PREDELAY.Init();
  PREDELAY.SetDelay(PREDELAY_TIME);

  for (size_t v = 0; v < N_VOICES; v++) {
    VOICES[v].Init(samplerate);
    // the table holds start positions as 0..1 phases, like the patch's
    // loadbang messages. turn them into samples.
    VOICES[v].pos *= BUFFER_LEN;
  }

  CLOCK.Init(88.0f / 60.0f, samplerate);  // the Lanka set ran at 88 BPM

  for (int ch = 0; ch < 2; ch++) {
    TONE[ch].Init();
    TONE[ch].SetFilterMode(OnePole::FILTER_MODE_LOW_PASS);
    LIMITERS[ch].Init();
  }
  MIX.Init(daisysp::CROSSFADE_CPOW);

  // check for FS1 held at power up -> stereo. Switch::Debounce() shifts one
  // reading into an 8 bit history per ms and Pressed() wants all 8 high, so
  // give it 10ms of readings. this also means the held press is already
  // "old" when audio starts, so it won't count as a RisingEdge and flip
  // the bypass. callbacks aren't registered yet so nothing else fires.
  for (int i = 0; i < 10; i++) {
    hw.ProcessDigitalControls();
    hw.DelayMs(1);
  }
  STEREO = hw.switches[Hothouse::FOOTSWITCH_1].Pressed();

  hw.RegisterFootswitchCallbacks(&FOOTSWITCH_CALLBACKS);

  led_enabled.Init(hw.seed.GetPin(Hothouse::LED_1), false);
  led_tempo.Init(hw.seed.GetPin(Hothouse::LED_2), false);

  hw.StartAdc();
  hw.StartAudio(AudioCallback);

  uint32_t last_tick = 0;
  uint32_t tempo_flash_until = 0;

  while (true) {
    hw.DelayMs(10);
    uint32_t now = System::GetNow();

    // pd: "clear" inlet. the audio keeps running while this happens, the
    // voices just fade into silence as the zeros go in.
    // ponytail: a tap of FS2 during the few ms this takes gets partly
    // wiped. not worth a lock.
    if (CLEAR_REQUESTED) {
      std::fill(BUFFER, BUFFER + BUFFER_LEN, 0.0f);
      CLEAR_REQUESTED = false;
    }

    // LED 1: on when the effect is on, blinking while recording
    bool enabled_led = !BYPASS;
    if (RECORDING) enabled_led = (now / 100) % 2;
    led_enabled.Set(enabled_led ? 1.0f : 0.0f);
    led_enabled.Update();

    // LED 2: 50ms flash on every clock tick
    if (TICK_COUNT != last_tick) {
      last_tick = TICK_COUNT;
      tempo_flash_until = now + 50;
    }
    led_tempo.Set(now < tempo_flash_until ? 1.0f : 0.0f);
    led_tempo.Update();

    // Hold *BOTH* footswitches for 2 s to reset to bootloader
    hw.CheckResetToBootloader();
  }

  return 0;
}
