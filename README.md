# West Coast Drum Synth

A Buchla-inspired west coast synthesis drum machine built as a VST3 plugin using the Steinberg VST3 SDK and VSTGUI.

## Overview

West Coast Drum Synth brings the organic, evolving timbres of west coast synthesis to percussive sound design. Inspired by Buchla's complex oscillators, wavefolders, and low pass gates, this instrument creates drums that sound alive -- from deep, resonant kicks to shimmering metallic hats and everything between.

## Synthesis Architecture

Each of the 6 drum voices features a complete west coast synthesis signal path:

- **Complex Oscillator** -- FM synthesis with configurable carrier/modulator ratio
- **Wavefolder** -- Multi-stage Buchla-inspired wavefolding with symmetry control, generating rich harmonic content without subtractive filtering
- **Low Pass Gate (LPG)** -- Vactrol-modeled combined VCA/VCF with nonlinear decay characteristics, producing the natural "plonk" that defines west coast percussion
- **Pitch Envelope** -- Exponential pitch sweeps for kicks, toms, and percussive transients
- **Noise Generator** -- White noise with filtering, plus an inharmonic metallic oscillator bank for cymbals and hi-hats
- **Drive & Waveshaping** -- Additional saturation and timbral variety

## Drum Voices

| Voice | MIDI Note | Character |
|-------|-----------|-----------|
| Kick | C2 (36) | Deep sine with pitch envelope and wavefolding |
| Snare | D2 (37) | FM body + filtered noise through LPG |
| Closed Hat | E2 (38) | Metallic inharmonic oscillators, short LPG |
| Open Hat | F2 (39) | Extended metallic tones, longer LPG decay |
| Tom | G2 (40) | Pitched body with envelope, moderate fold |
| Perc | A2 (41) | Complex wavefolded FM, resonant LPG |

## 16-Step Sequencer

- 16 steps per voice, 6 voice rows
- Per-step velocity
- Adjustable tempo (40-300 BPM) and swing
- Syncs to host transport
- Visual step grid with beat grouping

## Parameters Per Voice (16 each)

| Parameter | Description |
|-----------|-------------|
| Pitch | Base frequency of the oscillator |
| Decay | Amplitude envelope decay time |
| FM Amount | Frequency modulation depth |
| FM Ratio | Modulator/carrier frequency ratio |
| Fold | Wavefolding amount (harmonic richness) |
| Symmetry | Asymmetric wavefolding control |
| LPG | Low pass gate amount |
| LPG Decay | Vactrol release time |
| Resonance | LPG filter resonance |
| Noise | Noise/metallic oscillator mix |
| P.Env Amount | Pitch envelope depth |
| P.Env Decay | Pitch envelope speed |
| Drive | Input saturation |
| Pan | Stereo position |
| Shape | Additional waveshaping |
| Level | Voice output level |

## Presets

Factory presets included:
- Init
- West Coast Classic
- Buchla Beats
- Easel Groove
- Complex Waveform Kit
- LPG Percussion
- Metallic Modular
- Organic Machines
- Wavefolded Minimal
- FM Textures
- Voltage Controlled

## Building

### Requirements

- CMake 3.14+
- C++17 compiler (GCC or Clang)
- VST3 SDK (included as submodule)
- Linux: libx11-dev, libxcb-util-dev, libxcb-cursor-dev, libxcb-xkb-dev, libxkbcommon-dev, libxkbcommon-x11-dev, libfontconfig1-dev, libcairo2-dev, libgtkmm-3.0-dev, libfreetype-dev

### Build Instructions

```bash
# Clone with submodules
git clone --recursive https://github.com/sinelanguage/west-coast-drum-synth.git
cd west-coast-drum-synth

# Build
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --target WestCoastDrumSynth -j$(nproc)

# Plugin output: build/VST3/Release/WestCoastDrumSynth.vst3
```

### Install

Copy the `.vst3` bundle to your VST3 plugin directory:
- **Linux:** `~/.vst3/`
- **macOS:** `/Library/Audio/Plug-Ins/VST3/`
- **Windows:** `C:\Program Files\Common Files\VST3\`

## Interface

The GUI features a dark brushed-metal aesthetic inspired by Arturia hardware:
- Recessed panel sections with beveled edges
- Corona-lit knobs with orange indicators
- Color-coded step sequencer grid
- Gold accent typography and section headers
- Corner screw details and vent slots

## License

Copyright (c) 2026 Sine Language
