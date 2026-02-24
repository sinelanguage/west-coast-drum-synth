# West Coast Drum Synth - GUI Description

## Overview

The West Coast Drum Synth features a custom VSTGUI-based editor with a dark, hardware-inspired aesthetic. The interface is designed at 1280×760 pixels with an Arturia-style premium hardware look featuring brushed metal panels, gold accent typography, and intuitive control layout.

## Color Scheme

- **Backdrop**: Dark slate (#111318)
- **Panel**: Charcoal gray (#1f242d)  
- **Panel Edges**: Steel blue (#424a59)
- **Text Bright**: Off-white (#f4f6fa)
- **Text Soft**: Muted blue-gray (#9da6b5)
- **Accent Gold**: Warm gold (#d5a85b) - used for lane headers
- **Accent Blue**: Bright blue (#5cb4ffff) - used for select controls
- **Slider Track**: Dark gray (#2a3341)
- **Slider Frame**: Medium gray (#515e73)
- **Slider Value**: Gold (#e2b56b)

## Layout Structure

### Header Section (20, 16, 1240×50)
Large centered title banner with beveled edges:
**"WEST COAST DRUM SYNTH - BUCHLA INSPIRED MODULAR PERCUSSION"**

Subtitle (20, 70, 1240×22):
"West Coast timbre engine: FM transients + wavefolding + low-pass-gate decay + analog-noise accents."

### Global Transport / Mix Panel (20, 100, 1240×90)

**Controls:**
- **Master** - Horizontal slider (0-100%, default 80%)
- **Internal BPM** - Horizontal slider (60-180 BPM, default 120 BPM) - Blue accent
- **Swing** - Horizontal slider (0-100%, default 12%)
- **Preset** - Dropdown menu selector
- **RUN** - Toggle button (yellow/green "on" state)
- **FOLLOW HOST** - Toggle button for host transport sync

### Four Drum Lane Panels (Each 300×530 pixels)

Arranged horizontally across the interface:
1. **KICK LANE** (20, 210)
2. **SNARE LANE** (330, 210)
3. **HAT LANE** (640, 210)
4. **PERC LANE** (950, 210)

#### Per-Lane Control Layout

Each lane features identical control structure with **8 parameters** arranged in two rows:

**Top Row (4 vertical sliders, 150px tall):**
- **Tune** - Pitch control (-24 to +24 semitones, center-origin slider, default 0st)
- **Decay** - Envelope decay time (0.02-1.82s, default 0.50s) - Blue accent
- **Fold** - Wavefolding amount (0-100%, default 40%)
- **FM** - Frequency modulation depth (0-100%, default 35%)

**Bottom Row (4 vertical sliders, 150px tall):**
- **Noise** - Noise/metallic oscillator mix (0-100%, default 20%)
- **Drive** - Saturation/drive amount (0-100%, default 18%) - Blue accent  
- **Level** - Voice output level (0-100%, default 75%)
- **Pan** - Stereo position (-100% to +100%, center-origin slider, default 0%)

Each slider includes:
- Label above (soft gray text)
- Vertical slider with track, frame, and value indicator
- Numeric text edit field below for precise value entry (rounded rectangles)

## Control Styling

- **Sliders**: Dark track with framed edges, gold/blue value indicators depending on parameter
- **Center-origin sliders** (Tune, Pan): Value bar extends from center
- **Standard sliders**: Value bar extends from bottom
- **Buttons**: Rounded rectangles with gradient fills, state-dependent colors
- **Dropdowns**: Rounded rectangles with edge frames, centered text
- **Text fields**: Small rounded input boxes below each slider for direct value entry

## Visual Design Philosophy

The interface evokes vintage modular synthesizers and boutique hardware with:
- Recessed panel sections suggesting depth
- Beveled/rounded control edges
- Gold accent colors for headers and key parameters
- Blue accent highlights for time-based controls (Decay, Drive)
- Monochromatic base with strategic color highlights
- Clear visual hierarchy: Global → Lanes → Parameters
- Consistent spacing and alignment throughout

## Note on Editorhost Compatibility

**Important**: The VST3 SDK's `editorhost` utility has a known limitation where it does not initialize the VSTGUI platform factory. This causes plugins using VSTGUI (like West Coast Drum Synth) to crash with an assertion failure when attempting to create their editor:

```
Assertion 'gPlatformFactory' failed at platformfactory.cpp:39
```

### Workaround

To properly view and interact with the plugin GUI, use one of these alternatives:

1. **DAW Host**: Load the plugin in a full DAW (Reaper, Bitwig, Ardour, etc.)
2. **Carla**: Use the Carla plugin host which properly supports VSTGUI
3. **Custom Host**: Build a minimal host that calls `VSTGUI::initPlatform()` before loading plugins

The issue is specific to the `editorhost` sample application and does not affect the plugin's operation in standard music production environments.

## GUI File Location

The interface is defined in: `/workspace/resource/WestCoastEditor.uidesc`

This VSTGUI XML descriptor file contains all UI element positions, colors, control tags, and layout specifications.
