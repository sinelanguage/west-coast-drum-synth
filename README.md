# west-coast-drum-synth

West Coast Drum Synth is a VST3 drum-machine instrument built directly on the Steinberg VST3 SDK (no JUCE).

## Features

- Buchla / west coast-inspired percussive synthesis
- 5 lanes: Kick, Snare, Hat, Perc A, Perc B
- Per-lane controls: Tune, Decay, Fold, FM, Noise, Drive, Level, Pan
- Per-lane macro controls: Pitch Env/Decay, Attack Click, Transient Decay/Mix, Noise Tone/Decay, Noise Resonance, Noise Env Amount, Snap
- Internal 16-step sequencer with swing
- Host transport/tempo follow
- Factory presets
- Custom VSTGUI editor skin

## Project structure

- `source/WestCoastProcessor.*` - audio processing, sequencing, state/preset loading
- `source/WestCoastController.*` - parameter definitions, editor binding
- `source/engine/DrumVoice.*` - west coast drum voice DSP
- `source/engine/StepSequencer.*` - clock/swing/step timing
- `source/presets/FactoryPresets.*` - factory preset data
- `resource/WestCoastEditor.uidesc` - VSTGUI layout
- `source/factory.cpp` - VST3 class factory registration

## Build requirements

- CMake 3.22+
- C++20 toolchain
- VST3 SDK (auto-fetched if not supplied)

## Build (general)

```bash
cmake -S . -B build
cmake --build build --target WestCoastDrumSynth -j
```

Optional local SDK path:

```bash
export VST3_SDK_ROOT=/path/to/vst3sdk
cmake -S . -B build
cmake --build build --target WestCoastDrumSynth -j
```

## Build for macOS Apple Silicon (arm64)

The project now defaults to arm64 on macOS when no architecture is specified.

### Native arm64 (Apple Silicon)

```bash
cmake -S . -B build -DCMAKE_OSX_ARCHITECTURES=arm64
cmake --build build --target WestCoastDrumSynth -j
```

### Universal binary (arm64 + x86_64)

```bash
cmake -S . -B build -DWCSD_MAC_UNIVERSAL=ON
cmake --build build --target WestCoastDrumSynth -j
```

You can also force explicit arch selection via:

```bash
cmake -S . -B build -DWCSD_MAC_ARCHITECTURES=arm64
```

## Plugin output

The build produces a `.vst3` bundle target named **WestCoastDrumSynth**.

Install the bundle to:
- macOS: `/Library/Audio/Plug-Ins/VST3/`
- Linux: `~/.vst3/`
- Windows: `C:\Program Files\Common Files\VST3\`
