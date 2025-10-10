# Buzzbox Octa Squawker Controls Reference

Complete documentation of all hardware controls and their functions.

## Control Layout

### Knobs (6 total)

**Top Row (Left to Right)**:
- Knob 1: Input Gain
- Knob 2: Dry/Wet Mix
- Knob 3: Output Level

**Bottom Row (Left to Right)**:
- Knob 4: Context-dependent (see Switch 3)
- Knob 5: Context-dependent (see Switch 3)
- Knob 6: Context-dependent (see Switch 3)

### Toggle Switches (3 total)

**Physical Position → Software Value**:
- DOWN = case 2
- MIDDLE = case 1
- UP = case 0

### Footswitches (2 total)

**Left to Right**:
- FS1: Fuzz on/off
- FS2: Autowah/Octave on/off

### LEDs (2 total)

Both are **red** (not red/green):
- LED 1: Fuzz status
- LED 2: Autowah/Octave status

---

## Fixed Controls (Always Active)

### Knob 1: Input Gain
- **Range**: 0.5x to 2.0x
- **Function**: Boosts or attenuates input signal before processing
- **Typical Setting**: 12 o'clock (1.25x)
- **Notes**: Affects signal going into all effects

### Knob 2: Dry/Wet Mix
- **Range**: 0% to 100%
- **CCW (0%)**: Pure dry signal (bypass effects)
- **CW (100%)**: Pure wet signal (full effects)
- **Typical Setting**: 3 o'clock (75%)
- **Notes**: Blends processed and clean signal

### Knob 3: Output Level
- **Range**: 0% to 100%
- **Function**: Master output volume control
- **Typical Setting**: 12 o'clock to 2 o'clock
- **Notes**: Adjust to match bypass volume

---

## Context-Dependent Controls (Knobs 4-6)

Function depends on **Switch 3** position.

### Mode 1: Switch 3 UP - Fuzz Controls

#### Knob 4: Drive
- **Range**: 1x to 20x gain
- **Also Controls**: Fuzz intensity (0 to 1)
- **CCW**: Clean, minimal fuzz
- **CW**: Maximum aggressive fuzz
- **Notes**: Combined control for both gain and saturation amount

#### Knob 5: Tone
- **Range**: 100Hz to 1500Hz (lowpass filter)
- **CCW**: Dark, bassy tone (100Hz)
- **CW**: Brighter, cutting tone (1500Hz)
- **Notes**: Simple lowpass for vintage fuzz character

#### Knob 6: Gate Threshold
- **Range**: 0 to 1
- **CCW (0)**: Gate off, natural decay
- **CW**: Aggressive gate, staccato notes
- **Notes**: Eliminates noise between notes when CW

### Mode 2: Switch 3 MIDDLE - Autowah Controls

#### Knob 4: Attack/Release Speed
- **Attack Range**: 10ms to 200ms
- **Release Range**: 20ms to 400ms (2x attack)
- **CCW**: Slow, smooth filter sweep
- **CW**: Fast, snappy response
- **Notes**: Attack and release are linked

#### Knob 5: Threshold
- **Range**: Trigger sensitivity
- **Gate Level**: 0.05 + (knob × 0.15) = 0.05 to 0.20
- **CCW**: Triggers on quiet signals
- **CW**: Only triggers on loud signals
- **Notes**: How hard you need to play to trigger autowah

#### Knob 6: Filter Range
- **Base Range**: 100-1100Hz (CCW) to 300-3000Hz (CW)
- **CCW**: Lower, darker sweep
- **CW**: Higher, brighter sweep
- **Notes**: Shifts entire frequency range of autowah

### Mode 3: Switch 3 DOWN - Octave Controls

#### Knob 4: Octave Up Level
- **Range**: 0 to 1 (with 1.5x boost)
- **CCW**: No octave up
- **CW**: Maximum octave up
- **Notes**: Level of signal one octave above input

#### Knob 5: Octave Down Level
- **Range**: 0 to 1 (with 1.5x boost)
- **CCW**: No octave down
- **CW**: Maximum octave down
- **Notes**: Level of signal one octave below input

#### Knob 6: Overall Octave Mix
- **Range**: 0% to 100%
- **CCW**: Pure dry signal (octaves off)
- **CW**: Pure octave signal (dry off)
- **Notes**: Mixes dry with combined up/down octaves

---

## Toggle Switches

### Switch 1: Autowah Placement

Determines where autowah processes in signal chain:

#### UP Position (case 0): Before Fuzz
- **Signal Flow**: Input → Autowah → Octave → Fuzz
- **Character**: Autowah shapes signal going into fuzz
- **Use Case**: Filtered fuzz, vowel-like tones

#### MIDDLE Position (case 1): After Fuzz
- **Signal Flow**: Input → Octave → Fuzz → Autowah
- **Character**: Autowah filters already-fuzzed signal
- **Use Case**: Classic auto-wah on distortion

#### DOWN Position (case 2): After Everything
- **Signal Flow**: Input → Octave → Fuzz → Autowah
- **Character**: Autowah as final processing stage
- **Use Case**: Clean filtering, envelope control

### Switch 2: FS2 Effect Selection

Determines what FS2 activates:

#### UP Position (case 0): Both Autowah + Octave
- Pressing FS2 toggles both effects simultaneously
- **Use Case**: Full effect stack

#### MIDDLE Position (case 1): Autowah Only
- Pressing FS2 toggles autowah only
- Octave remains off
- **Use Case**: Just envelope filter

#### DOWN Position (case 2): Octave Only
- Pressing FS2 toggles octave only
- Autowah remains off
- **Use Case**: Just octave generation

### Switch 3: Knobs 4-6 Function Mode

See "Context-Dependent Controls" section above.

---

## Footswitches

### FS1 (Left): Fuzz Toggle
- **Press**: Toggle fuzz on/off
- **LED 1**: Lights when fuzz active
- **Hold 2+ seconds**: Enter bootloader mode

### FS2 (Right): Autowah/Octave Toggle
- **Press**: Toggle effects per Switch 2 setting
- **LED 2**: Lights when autowah/octave active
- **Behavior**: See Switch 2 section above

---

## Touch-to-Activate Behavior

When you switch **Switch 3** to change knobs 4-6 function:

1. **Parameters hold their current values**
2. **Knob positions don't immediately affect sound**
3. **Move any knob 4-6** to activate it
4. **Once activated, knob works normally**

**Example**:
- Switch 3 in UP (fuzz mode), Knob 4 at 75% (high drive)
- Move Switch 3 to MIDDLE (autowah mode)
- Drive stays at high setting
- Move Knob 4 → it now controls autowah speed from current position

---

## Typical Patches

### Classic Fuzz
- Switch 1: MIDDLE (autowah after fuzz)
- Switch 2: MIDDLE (autowah only)
- Switch 3: UP (fuzz controls)
- FS1: ON (fuzz active)
- FS2: OFF
- Knob 1: 12 o'clock
- Knob 2: 3 o'clock
- Knob 3: 1 o'clock
- Knob 4 (drive): 2 o'clock
- Knob 5 (tone): 10 o'clock
- Knob 6 (gate): 9 o'clock

### Auto-Wah Lead
- Switch 1: UP (autowah before fuzz)
- Switch 2: MIDDLE (autowah only)
- Switch 3: MIDDLE (autowah controls)
- FS1: ON (fuzz active)
- FS2: ON (autowah active)
- Knob 4 (speed): 2 o'clock (snappy)
- Knob 5 (threshold): 11 o'clock
- Knob 6 (range): 1 o'clock

### Octave + Fuzz
- Switch 1: DOWN (autowah after)
- Switch 2: DOWN (octave only)
- Switch 3: DOWN (octave controls)
- FS1: ON (fuzz active)
- FS2: ON (octave active)
- Knob 4 (up level): 2 o'clock
- Knob 5 (down level): 10 o'clock
- Knob 6 (mix): 12 o'clock

---

## Quick Reference Card

| Control | Function | Range |
|---------|----------|-------|
| Knob 1 | Input Gain | 0.5x - 2.0x |
| Knob 2 | Mix | 0% - 100% |
| Knob 3 | Output | 0% - 100% |
| **SW3 UP:** | | |
| - Knob 4 | Drive | 1x - 20x |
| - Knob 5 | Tone | 100Hz - 1500Hz |
| - Knob 6 | Gate | 0 - 1 |
| **SW3 MID:** | | |
| - Knob 4 | Speed | 10ms - 200ms |
| - Knob 5 | Threshold | Low - High |
| - Knob 6 | Range | 100Hz - 3000Hz |
| **SW3 DOWN:** | | |
| - Knob 4 | Oct Up | 0 - 1 |
| - Knob 5 | Oct Down | 0 - 1 |
| - Knob 6 | Oct Mix | 0% - 100% |

---

## Tips and Tricks

### Getting Started
1. Start with all knobs at 12 o'clock
2. Set Switch 3 to UP (fuzz mode)
3. Press FS1 to enable fuzz
4. Adjust Knob 4 (drive) for desired fuzz amount
5. Adjust Knob 5 (tone) for brightness
6. Use Knob 2 to blend in dry signal

### Dialing in Autowah
1. Switch 3 to MIDDLE
2. Press FS2 to enable autowah
3. Start with Knob 4 (speed) at 2 o'clock for snappy response
4. Adjust Knob 5 (threshold) until filter triggers consistently
5. Use Knob 6 (range) to shift frequency sweep

### Octave Tips
1. Octaves work best with clean, sustained notes
2. Try octave up alone for organ-like sounds
3. Mix octave down with fuzz for heavy bass tones
4. Use Knob 6 to blend octaves with dry signal

### Touch-to-Activate Tips
- Leave frequently-used settings in place
- Switch modes without affecting sound
- Move knob slightly to "grab" control
- Visual feedback: watch LED indicators

---

## Troubleshooting

### Controls Not Responding
- Check knob connections
- Verify hardware is powered
- Test with bypass (Knob 2 fully CCW)

### Wrong Parameters Changing
- Check Switch 3 position
- Verify you're in correct mode
- Touch-to-activate may not be engaged yet

### No LED Response
- Check footswitch connections
- Verify LED polarity (both red)
- Test basic hardware functionality

---

## Control Voltage Ranges

For developers modifying parameters:

```cpp
// Input gain
float input_gain = 0.5f + (knobValues[0] * 1.5f);  // 0.5x to 2.0x

// Drive (gain + intensity combined)
float gain = 1.0f + (drive_amount * 19.0f);  // 1x to 20x
float intensity = drive_amount;  // 0 to 1

// Tone
float tone_freq = 100.0f + (tone_knob * 1400.0f);  // 100Hz to 1500Hz

// Autowah speed
float attack = 10.0f + (speed_knob * 190.0f);  // 10ms to 200ms
float release = attack * 2.0f;  // 20ms to 400ms

// Autowah threshold
float gate_level = 0.05f + (threshold_knob * 0.15f);  // 0.05 to 0.20

// Autowah range
float range_min = 100.0f + (range_knob * 200.0f);  // 100Hz to 300Hz
float range_max = 1100.0f + (range_knob * 1900.0f);  // 1100Hz to 3000Hz

// Octave levels (with boost)
float octave_up_level = octave_up_knob * 1.5f;  // 0 to 1.5
float octave_down_level = octave_down_knob * 1.5f;  // 0 to 1.5
```
