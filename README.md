# west-coast-drum-synth

West Coast Drum Synth is a VST3 drum-machine instrument built directly on the Steinberg VST3 SDK (no JUCE).

## What it is

- **Buchla / west coast-inspired drum engine**  
  Per-lane FM transient shaping, wavefolding, low-pass-gate style envelope behavior, transient noise and saturation.
- **4-lane modular drum voice layout**  
  Kick, Snare, Hat, Perc lanes with individual Tune / Decay / Fold / FM / Noise / Drive / Level / Pan.
- **Integrated 16-step sequencer**  
  Internal clock plus optional host transport/tempo following.
- **Factory preset system**  
  Multiple curated rhythmic + timbral presets loaded via a preset selector parameter.
- **Custom VSTGUI editor skin**  
  Dark, hardware-inspired paneling and lane-strip layout in a high-polish Arturia-style visual direction.

## Project structure

- `source/WestCoastProcessor.*` - audio engine host integration, sequencing, preset loading
- `source/WestCoastController.*` - parameters, editor hookup, state restoration
- `source/engine/DrumVoice.*` - west coast percussive synthesis voice
- `source/engine/StepSequencer.*` - timing, swing, transport sync
- `source/presets/FactoryPresets.*` - factory patches and pattern grids
- `resource/WestCoastEditor.uidesc` - custom VSTGUI interface layout
- `source/factory.cpp` - VST3 class factory registration

## Building

### Requirements

- CMake 3.22+
- C++20 compiler
- Steinberg VST3 SDK (auto-fetched if not provided locally)

### Build command

```bash
cmake -S . -B build
cmake --build build -j
```

Optional local SDK path:

```bash
export VST3_SDK_ROOT=/path/to/vst3sdk
cmake -S . -B build
cmake --build build -j
```

The build produces a `.vst3` bundle target named **WestCoastDrumSynth**.