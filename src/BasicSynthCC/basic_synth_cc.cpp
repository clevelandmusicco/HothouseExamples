// BasicSynthCC for Hothouse DIY DSP Platform
// Copyright (C) 2026 Cleveland Music Co. <code@clevelandmusicco.com>
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
//
// Same synth as BasicSynth, but the four knob-controlled params are also
// driven by MIDI CC 14-19 (see hothouse.h): turn a knob or send its CC,
// last one to move wins. Demonstrates Hothouse's baked-in CC->knob mapping;
// see hothouse.h/.cpp for the mechanism itself.

#include "daisysp-lgpl.h"
#include "daisysp.h"
#include "hothouse.h"

using namespace clevelandmusicco;
using namespace daisysp;
using namespace daisy;

Hothouse hw;
Oscillator osc;
MoogLadder flt;
HothouseParameter p_freq, p_res, p_attack, p_decay;
AdEnv env;

bool note_active = false;

constexpr int waveforms[] = {
    Oscillator::WAVE_SIN,              // TOGGLESWITCH_UP
    Oscillator::WAVE_POLYBLEP_SQUARE,  // TOGGLESWITCH_MIDDLE
    Oscillator::WAVE_POLYBLEP_SAW      // TOGGLESWITCH_DOWN
};

// Registered with hw.RegisterMidiEventCallback(); receives whatever
// ProcessMidi() doesn't consume itself (i.e. everything but CC/PC).
void HandleMidiEvent(MidiEvent msg) {
  switch (msg.type) {
    case NoteOn: {
      auto note_msg = msg.AsNoteOn();
      if (note_msg.velocity != 0) {
        osc.SetFreq(mtof(note_msg.note));
        env.Trigger();
        note_active = true;
      } else {
        note_active = false;
      }
    } break;
    case NoteOff: {
      note_active = false;
    } break;
    default:
      break;
  }
}

void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out,
                   size_t size) {
  hw.ProcessAllControls();

  env.SetTime(ADENV_SEG_ATTACK, p_attack.Process());
  env.SetTime(ADENV_SEG_DECAY, p_decay.Process());

  osc.SetWaveform(
      waveforms[hw.GetToggleswitchPosition(Hothouse::TOGGLESWITCH_1)]);

  // Knobs are control rate, so scale them once per block; the LOGARITHMIC
  // curve costs an expf() per call.
  const float cutoff = p_freq.Process();
  flt.SetRes(p_res.Process());

  for (size_t i = 0; i < size; ++i) {
    float env_out = env.Process();

    if (!env.IsRunning()) {
      note_active = false;
    }

    // Modulate the filter cutoff using the envelope
    flt.SetFreq(cutoff * env_out);

    out[0][i] = out[1][i] = flt.Process(osc.Process()) * env_out;
  }
}

int main() {
  hw.Init();
  hw.SetAudioBlockSize(48);
  hw.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_48KHZ);

  p_freq.Init(&hw, Hothouse::KNOB_1, 20.0f, 20000.0f, Parameter::LOGARITHMIC);
  p_res.Init(&hw, Hothouse::KNOB_2, 0.0f, 1.0f, Parameter::LINEAR);
  p_attack.Init(&hw, Hothouse::KNOB_3, 0.001f, 0.25f, Parameter::LOGARITHMIC);
  p_decay.Init(&hw, Hothouse::KNOB_4, 0.05f, 2.0f, Parameter::LOGARITHMIC);

  hw.StartMidi();
  hw.RegisterMidiEventCallback(HandleMidiEvent);

  osc.Init(hw.AudioSampleRate());
  osc.SetWaveform(Oscillator::WAVE_POLYBLEP_SAW);  // Initial default waveform

  flt.Init(hw.AudioSampleRate());
  flt.SetRes(0.7);

  env.Init(hw.AudioSampleRate());

  hw.StartAdc();
  hw.StartAudio(AudioCallback);

  // 1 ms rather than the usual 10: at 10 ms, note-on jitter is audible and a
  // dense CC stream can back up in the USB MIDI FIFO.
  while (true) {
    hw.ProcessMidi();
    hw.DelayMs(1);
    hw.CheckResetToBootloader();
  }
  return 0;
}
