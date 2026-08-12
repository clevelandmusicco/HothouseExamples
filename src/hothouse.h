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

/** Factory default MIDI channel: 0 = omni, 1-16 = that channel. Override per
 * effect with -DHOTHOUSE_MIDI_CHANNEL=n. A channel learned at boot and saved
 * to QSPI takes precedence until the FOOTSWITCH_2 boot gesture clears it. */
#ifndef HOTHOUSE_MIDI_CHANNEL
#define HOTHOUSE_MIDI_CHANNEL 0
#endif

/** Where the settings block lives on the 8 MB QSPI chip. Kept well clear of
 * address 0 so it survives a program flashed there by the Daisy bootloader. */
#ifndef HOTHOUSE_SETTINGS_QSPI_OFFSET
#define HOTHOUSE_SETTINGS_QSPI_OFFSET 0x400000
#endif

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

  /** Non-volatile settings, persisted to QSPI. Bump kSettingsVersion when the
   * layout changes; a mismatched block is discarded rather than reinterpreted.
   * \note PersistentStorage compares before erasing, hence operator!=. */
  struct Settings {
    uint16_t version;
    uint8_t midi_channel; /**< 0 = omni, 1-16 = that channel */
    uint8_t reserved;     /**< Pads to a word; free for the next setting */

    bool operator==(const Settings& other) const {
      return version == other.version && midi_channel == other.midi_channel &&
             reserved == other.reserved;
    }
    bool operator!=(const Settings& other) const { return !(*this == other); }
  };

  /** MIDI channel value meaning "listen on all channels". */
  static const uint8_t MIDI_CHANNEL_OMNI = 0;

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

  /** Level read for footswitch functions that are held, e.g. a freeze or a
   * momentary boost. Use this and GetFootswitchRisingEdge() rather than
   * switches[FOOTSWITCH_n] directly; the raw Switch is physical-only, so an
   * effect reading it silently ignores the footswitch's MIDI CC.
   * \param footswitch Which footswitch to check (FOOTSWITCH_1 or
   * FOOTSWITCH_2; anything else logs an error and returns false).
   * \return true if physically held OR its MIDI CC override is currently
   * "on" (CC value >= 64, the standard MIDI button convention). */
  bool GetFootswitchPressed(Switches footswitch);

  /** Edge read for footswitch functions that latch, e.g. the `bypass ^= ...`
   * idiom most effects use. Same physical-OR-CC rule as
   * GetFootswitchPressed().
   * \param footswitch Which footswitch to check (FOOTSWITCH_1 or
   * FOOTSWITCH_2; anything else logs an error and returns false).
   * \return true for one ProcessDigitalControls() cycle after either the
   * physical switch or its MIDI CC crosses into the pressed state. */
  bool GetFootswitchRisingEdge(Switches footswitch);

  /** Whole-pedal bypass state, owned here so MIDI and the footswitch can't
   * disagree about it. CC 25 sets it absolutely (>= 64 engaged, < 64
   * bypassed); the effect flips it from the footswitch. Nothing in this class
   * mutes audio on its own -- the effect still decides what "bypassed" means
   * for its own signal path.
   * \return true when the pedal should pass the dry signal through. */
  bool GetBypass();

  /** Set the bypass state outright, e.g. an effect's power-on default.
   * \note Call from main() before StartAudio(), or from inside the audio
   * callback. Calling it from the main loop while audio is running races the
   * CC adoption in ProcessDigitalControls().
   * \param bypassed true to bypass, false to engage. */
  void SetBypass(bool bypassed);

  /** Flip the bypass state; the footswitch half of the pair, e.g.
   *   if (hw.GetFootswitchRisingEdge(FOOTSWITCH_2)) hw.ToggleBypass();
   * \note Same calling-context rule as SetBypass(). */
  void ToggleBypass();

  /** Check whether FOOTSWITCH_1 and FOOTSWITCH_2 have both been held down
   * simultaneously for 2 seconds and, if so, call System::ResetToBootloader().
   * This has the same effect as pressing the Daisy Seed RESET and BOOT buttons
   * to get into flashable (DFU) mode. \note The LEDs will alternately flash
   * three times before the reset. */
  void CheckResetToBootloader();

  /** Register/Deregister footswitch press callbacks. This provides an
   * alternative way of handling foot switch presses and allows effects to make
   * use of double and long presses. Fed by physical stomps and by the
   * footswitch MIDI CCs alike, so a long press needs the CC to sit above 64
   * for the duration -- set the controller to momentary, not latching.
   * \param callbacks A pointer to the struct that defines the callbacks or NULL
   * to deregister all callbacks.
   */
  void RegisterFootswitchCallbacks(FootswitchCallbacks *callbacks);

  /** Signature for MIDI messages Hothouse doesn't consume itself, i.e.
   * everything but the CCs in its built-in control map. */
  using MidiEventCallback = void (*)(MidiEvent event);

  /** Init and start listening for MIDI over USB (device mode: the host
   * computer enumerates the Hothouse as a class-compliant USB MIDI port).
   * Also loads the saved MIDI channel and runs the boot-time channel gestures
   * (FOOTSWITCH_1 held = learn, FOOTSWITCH_2 held = reset to omni), so call it
   * from main() before StartAudio(): it blinks the LEDs and blocks for up to
   * 10 s if a gesture is held. */
  void StartMidi();

  /** \return The MIDI channel currently being listened to: MIDI_CHANNEL_OMNI
   * (0) for all channels, otherwise 1-16. */
  uint8_t GetMidiChannel() const;

  /** Set the MIDI channel and persist it to QSPI, surviving power cycles and
   * reflashes. Values above 16 are ignored.
   * \note Blocking flash erase/write; call before StartAudio(), never from the
   * audio callback.
   * \param channel MIDI_CHANNEL_OMNI (0) for all channels, or 1-16. */
  void SetMidiChannel(uint8_t channel);

  /** Drain pending MIDI messages; call once per main loop iteration.
   * Only CCs in the built-in control map are consumed; everything else,
   * Program Change included, forwards to the registered MidiEventCallback.
   * Messages carrying a channel are dropped unless it matches GetMidiChannel();
   * clock, sysex and the rest of System Common/Real Time always get through.
   * Sole drain point; don't touch midi_ elsewhere or the two will starve. */
  void ProcessMidi();

  /** Register/deregister the callback for MIDI messages not consumed by the
   * built-in CC map. Pass nullptr to deregister.
   * \param callback Function to call for each unconsumed MIDI event.
   */
  void RegisterMidiEventCallback(MidiEventCallback callback);

  /** Most recently received MIDI Program Change number.
   * \return 0-127 if a Program Change has been received since boot, -1
   * otherwise. */
  int16_t GetProgramNumber();

  DaisySeed seed; /**< & */

  AnalogControl knobs[KNOB_LAST]; /**< & */

  /** Raw, physical-only switch state. For the footswitches, prefer
   * GetFootswitchPressed() / GetFootswitchRisingEdge(); for the toggles,
   * GetToggleswitchPosition(). Those blend in MIDI CC, these don't. */
  Switch switches[SWITCH_LAST];

 private:
  void SetHidUpdateRates();
  void InitSwitches();
  void InitAnalogControls();
  ToggleswitchPosition GetLogicalSwitchPosition(const Switch& up,
                                                const Switch& down);
  ToggleswitchPosition ReadPhysicalToggleswitchPosition(Toggleswitch tsw);
  void ProcessFootswitchPresses(Switches footswitch);
  void ProcessDigitalCcOverrides();
  bool HandleControlChange(const daisy::ControlChangeEvent& cc);
  bool FootswitchIndex(Switches footswitch, size_t* idx);
  void LoadSettings();
  void RestoreFactorySettings();
  void RunMidiChannelGestures();
  bool RunMidiChannelLearn();
  void DebounceFootswitches(uint32_t duration_ms);
  void BlinkChannel(uint8_t channel);

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
  MidiEventCallback midi_event_callback_ = nullptr;

  // Read once in StartMidi(), so effects that never ask for MIDI don't spin up
  // QSPI storage at all. Declared after seed, which owns the qspi handle.
  daisy::PersistentStorage<Settings> settings_storage_{seed.qspi};
  bool settings_loaded_ = false;

  // LoadSettings() is what actually applies HOTHOUSE_MIDI_CHANNEL, after
  // range-checking it. Starting omni keeps an out-of-range define from ever
  // being visible here.
  uint8_t midi_channel_ = MIDI_CHANNEL_OMNI;

  // MIDI CC state crosses contexts: ProcessMidi() runs in the main loop, the
  // rest runs in the audio ISR. Every word below has exactly one writer, so
  // neither side can lose the other's read-modify-write.
  volatile float knob_cc_value_[KNOB_LAST] = {};
  volatile uint8_t knob_cc_seq_[KNOB_LAST] = {};
  volatile ToggleswitchPosition toggle_cc_value_[TOGGLESWITCH_LAST] = {};
  volatile uint8_t toggle_cc_seq_[TOGGLESWITCH_LAST] = {};
  volatile bool footswitch_cc_pressed_[2] = {};
  volatile uint8_t footswitch_cc_edge_seq_[2] = {};
  volatile bool bypass_cc_bypassed_ = false;
  volatile uint8_t bypass_cc_seq_ = 0;
  volatile int16_t program_number_ = -1;

  // ISR-side only: last sequence number consumed, plus the derived state the
  // accessors read.
  uint8_t knob_cc_seen_[KNOB_LAST] = {};
  float knob_cc_last_raw_[KNOB_LAST] = {};
  bool knob_cc_active_[KNOB_LAST] = {};
  uint8_t toggle_cc_seen_[TOGGLESWITCH_LAST] = {};
  ToggleswitchPosition toggle_last_physical_[TOGGLESWITCH_LAST] = {};
  bool toggle_cc_active_[TOGGLESWITCH_LAST] = {};
  uint8_t footswitch_cc_edge_seen_[2] = {};
  bool footswitch_cc_rising_edge_[2] = {};
  uint8_t bypass_cc_seen_ = 0;

  // ISR-owned, but read from the main loop (LED updates), so volatile.
  volatile bool bypassed_ = false;
};

/** Drop-in replacement for daisy::Parameter that reads through
 * Hothouse::GetKnobValue() instead of an AnalogControl's ADC pointer
 * directly, so it picks up MIDI CC overrides for free. Curve math matches
 * daisy::Parameter exactly (see libDaisy/src/hid/parameter.cpp). */
class HothouseParameter {
 public:
  HothouseParameter() = default;
  ~HothouseParameter() = default;

  /** \param hw Pointer to the Hothouse instance owning the knob.
   * \param knob Which knob to read.
   * \param min Bottom of range (when input is 0.0).
   * \param max Top of range (when input is 1.0).
   * \param curve Scaling curve for the input->output transformation.
   */
  void Init(Hothouse* hw, Hothouse::Knob knob, float min, float max,
            Parameter::Curve curve);

  /** Processes the input signal; call once per audio block. */
  float Process();

  /** Current value without processing another sample. */
  inline float Value() { return val_; }

 private:
  Hothouse* hw_ = nullptr;
  Hothouse::Knob knob_ = Hothouse::KNOB_1;
  float pmin_ = 0.0f, pmax_ = 0.0f;
  float lmin_ = 0.0f, lmax_ = 0.0f;
  float val_ = 0.0f;
  Parameter::Curve curve_ = Parameter::LINEAR;
};

}  // namespace clevelandmusicco
