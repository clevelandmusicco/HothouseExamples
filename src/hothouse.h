// Hardware proxy for Hothouse DIY DSP Platform
// Copyright (C) 2024  Cleveland Music Co.  <code@clevelandmusicco.com>
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

#include "daisy_seed.h"

using daisy::AdcChannelConfig;
using daisy::AnalogControl;
using daisy::AudioHandle;
using daisy::DaisySeed;
using daisy::Led;
using daisy::MidiEvent;
using daisy::MidiUsbHandler;
using daisy::Parameter;
using daisy::Pin;
using daisy::SaiHandle;
using daisy::Switch;

namespace clevelandmusicco {
class Hothouse {
 public:
  /** Switches */
  enum Switches {
    SWITCH_1_UP,   /** Up position of toggleswitch 1 */
    SWITCH_1_DOWN, /** Down position of toggleswitch 1 */
    SWITCH_2_UP,   /** Up position of toggleswitch 2 */
    SWITCH_2_DOWN, /** Down position of toggleswitch 2 */
    SWITCH_3_UP,   /** Up position of toggleswitch 3 */
    SWITCH_3_DOWN, /** Down position of toggleswitch 3 */
    FOOTSWITCH_1,  /** Footswitch 1 */
    FOOTSWITCH_2,  /** Footswitch 2 */
    SWITCH_LAST,   /**< Last enum item */
  };

  /** Knobs */
  enum Knob {
    KNOB_1,    /**< & */
    KNOB_2,    /**< & */
    KNOB_3,    /**< & */
    KNOB_4,    /**< & */
    KNOB_5,    /**< & */
    KNOB_6,    /**< & */
    KNOB_LAST, /**< & */
  };

  /** footswitch leds */
  enum Led {
    LED_1 = 22, /**< & */
    LED_2 = 23, /**< & */
    LED_LAST,   /**< & */
  };

  enum ToggleswitchPosition {
    TOGGLESWITCH_UP,
    TOGGLESWITCH_MIDDLE,
    TOGGLESWITCH_DOWN,
    TOGGLESWITCH_UNKNOWN,
  };

  enum Toggleswitch {
    TOGGLESWITCH_1,
    TOGGLESWITCH_2,
    TOGGLESWITCH_3,
    TOGGLESWITCH_LAST, /**< & */
  };

  struct FootswitchCallbacks {
    /** Called when a single footswitch press is detected. */
    void (*HandleNormalPress)(Switches footswitch);

    /** Called when a double footswitch press is detected. */
    void (*HandleDoublePress)(Switches footswitch);

    /** Called when a long footswitch press is detected. */
    void (*HandleLongPress)(Switches footswitch);
  };

  // Constructor and Destructor
  Hothouse() = default;
  ~Hothouse() = default;

  /** Initialize Hothouse */
  void Init(bool boost = false);

  /**
     Wait before moving on.
     \param del Delay time in ms.
   */
  void DelayMs(size_t del);

  /** Starts the callback
  \param cb Interleaved callback function
  */
  void StartAudio(AudioHandle::InterleavingAudioCallback cb);

  /** Starts the callback
  \param cb multichannel callback function
  */
  void StartAudio(AudioHandle::AudioCallback cb);

  /**
     Switch callback functions
     \param cb New interleaved callback function.
  */
  void ChangeAudioCallback(AudioHandle::InterleavingAudioCallback cb);

  /**
     Switch callback functions
     \param cb New multichannel callback function.
  */
  void ChangeAudioCallback(AudioHandle::AudioCallback cb);

  /** Stops the audio if it is running. */
  void StopAudio();

  /** Updates the Audio Sample Rate, and reinitializes.
   ** Audio must be stopped for this to work.
   */
  void SetAudioSampleRate(SaiHandle::Config::SampleRate samplerate);

  /** Returns the audio sample rate in Hz as a floating point number.
   */
  float AudioSampleRate();

  /** Sets the number of samples processed per channel by the audio callback.
     \param size Audio block size
   */
  void SetAudioBlockSize(size_t size);

  /** Returns the number of samples per channel in a block of audio. */
  size_t AudioBlockSize();

  /** Returns the rate in Hz that the Audio callback is called */
  float AudioCallbackRate();

  /** Start analog to digital conversion. */
  void StartAdc();

  /** Stops Transfering data from the ADC */
  void StopAdc();

  /** Call at the same frequency as controls are read for stable readings.*/
  void ProcessAnalogControls();

  /** Process Analog and Digital Controls */
  inline void ProcessAllControls() {
    ProcessAnalogControls();
    ProcessDigitalControls();
  }

  /** Get value per knobs.
  \param k Which knobs to get
  \return Floating point knobs position.
  */
  float GetKnobValue(Knob k);

  /** Process digital controls */
  void ProcessDigitalControls();

  /** Get the current position of a toggleswitch (up, down, or middle).
  \param tsw Which toggleswitch to interogate (TOGGLESWITCH_1, TOGGLESWITCH_2,
  or TOGGLESWITCH_3) \return TOGGLESWITCH_UP (0), TOGGLESWITCH_MIDDLE (1), or
  TOGGLESWITCH_DOWN (2). \note If the toggleswitch in question is ON-ON (rather
  than ON-OFF-ON), TOGGLESWITCH_MIDDLE can never be the return value. Write
  your code with this in mind. \note Also blends in a MIDI CC override the
  same way GetKnobValue() does; see hothouse.cpp.
  */
  ToggleswitchPosition GetToggleswitchPosition(Toggleswitch tsw);

  /** \param footswitch Which footswitch to check (FOOTSWITCH_1/2).
   * \return true if physically pressed OR its MIDI CC override is currently
   * "on" (CC value >= 64, the standard MIDI button convention). Doesn't
   * feed RegisterFootswitchCallbacks or CheckResetToBootloader, since MIDI
   * shouldn't be able to trigger DFU reset. */
  bool GetFootswitchPressed(Switches footswitch);

  /** Check whether FOOTSWITCH_1 and FOOTSWITCH_2 have both been held down
   * simultaneously for 2 seconds and, if so, call System::ResetToBootloader().
   * This has the same effect as pressing the Daisy Seed RESET and BOOT buttons
   * to get into flashable (DFU) mode. \note The LEDs will alternately flash
   * three times before the reset. */
  void CheckResetToBootloader();

  /** Register/Deregister footswitch press callbacks. This provides an
   * alternative way of handling foot switch presses and allows effects to make
   * use of double and long presses.
   * \param callbacks A pointer to the struct that defines the callbacks or NULL
   * to deregister all callbacks.
   */
  void RegisterFootswitchCallbacks(FootswitchCallbacks *callbacks);

  /** Signature for MIDI messages Hothouse doesn't consume itself (anything
   * other than ControlChange/ProgramChange, e.g. NoteOn/NoteOff). */
  using MidiEventCallback = void (*)(MidiEvent event);

  /** Init and start listening for MIDI over USB (device mode: the host
   * computer enumerates the Hothouse as a class-compliant USB MIDI port). */
  void StartMidi();

  /** Drain pending MIDI messages; call once per main loop iteration.
   * ControlChange/ProgramChange update internal state, everything else
   * forwards to the registered MidiEventCallback. Sole drain point;
   * don't touch midi_ elsewhere, or the two consumers will starve. */
  void ProcessMidi();

  /** Register/deregister the callback for MIDI messages not handled by the
   * built-in CC/PC logic. Pass NULL to deregister.
   * \param callback Function to call for each unconsumed MIDI event.
   */
  void RegisterMidiEventCallback(MidiEventCallback callback);

  /** Most recently received MIDI Program Change number.
   * \return 0-127 if a Program Change has been received since boot, -1
   * otherwise. (Not std::optional: this toolchain builds with
   * -std=gnu++14, and libstdc++'s <optional> compiles out under it.) */
  int16_t GetProgramNumber();

  DaisySeed seed; /**< & */

  AnalogControl knobs[KNOB_LAST]; /**< & */
  Switch switches[SWITCH_LAST];   /**< & */

 private:
  void SetHidUpdateRates();
  void InitSwitches();
  void InitAnalogControls();
  ToggleswitchPosition GetLogicalSwitchPosition(Switch up, Switch down);
  void ProcessFootswitchPresses(Switches footswitch);

  uint32_t footswitch_start_time[2] = {0, 0};
  uint32_t footswitch_last_press_time[2] = {0, 0};
  bool footswitch_last_state[2] = {false, false};
  uint8_t footswitch_press_count[2] = {0, 0};
  bool footswitch_long_press_triggered[2] = {false, false};
  uint32_t dfu_start_time_ = 0;
  static const uint32_t HOLD_THRESHOLD_MS = 2000;
  static const uint32_t DOUBLE_PRESS_THRESHOLD_MS = 600;

  inline uint16_t* adc_ptr(const uint8_t chn) { return seed.adc.GetPtr(chn); }

  FootswitchCallbacks *footswitchCallbacks = NULL;

  MidiUsbHandler midi_;
  MidiEventCallback midi_event_callback_ = NULL;
  float knob_cc_value_[KNOB_LAST] = {};
  float knob_cc_last_raw_[KNOB_LAST] = {};
  bool knob_cc_active_[KNOB_LAST] = {};
  ToggleswitchPosition toggle_cc_value_[TOGGLESWITCH_LAST] = {};
  ToggleswitchPosition toggle_last_physical_[TOGGLESWITCH_LAST] = {};
  bool toggle_cc_active_[TOGGLESWITCH_LAST] = {};
  bool footswitch_cc_pressed_[2] = {};
  int16_t program_number_ = -1;
};

/** Drop-in replacement for daisy::Parameter that reads through
 * Hothouse::GetKnobValue() instead of an AnalogControl's ADC pointer
 * directly, so it picks up MIDI CC overrides for free. Curve math matches
 * daisy::Parameter exactly (see libDaisy/src/hid/parameter.cpp). */
class HothouseParameter {
 public:
  HothouseParameter() = default;
  ~HothouseParameter() = default;

  /** \param hw The Hothouse instance owning the knob.
   * \param knob Which knob to read.
   * \param min Bottom of range (when input is 0.0).
   * \param max Top of range (when input is 1.0).
   * \param curve Scaling curve for the input->output transformation.
   */
  void Init(Hothouse &hw, Hothouse::Knob knob, float min, float max,
            Parameter::Curve curve);

  /** Processes the input signal; call once per audio block. */
  float Process();

  /** Current value without processing another sample. */
  inline float Value() { return val_; }

 private:
  Hothouse *hw_ = nullptr;
  Hothouse::Knob knob_ = Hothouse::KNOB_1;
  float pmin_ = 0.0f, pmax_ = 0.0f;
  float lmin_ = 0.0f, lmax_ = 0.0f;
  float val_ = 0.0f;
  Parameter::Curve curve_ = Parameter::LINEAR;
};

}  // namespace clevelandmusicco
