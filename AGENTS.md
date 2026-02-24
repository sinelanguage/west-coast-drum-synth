# AGENTS.md

## Cursor Cloud specific instructions

### Project overview

West Coast Drum Synth is a C++20 VST3 audio plugin built with CMake and the Steinberg VST3 SDK (fetched via CMake FetchContent). There are no web services, databases, or Docker dependencies. See `README.md` for synthesis architecture details.

### Git workflow (important)

Use this flow so Cloud/agent branches and `main` stay predictable:

1. Start from the latest main:
   ```bash
   git fetch origin main
   git checkout main
   git pull origin main
   ```
2. Create a task branch:
   ```bash
   git checkout -b cursor/<task-name>
   ```
3. Make changes, then push:
   ```bash
   git push -u origin cursor/<task-name>
   ```
4. Merge through PR into `main` (preferred), then sync local:
   ```bash
   git checkout main
   git pull origin main
   ```

If `main` and a cursor branch diverge accidentally, reconcile by merging the cursor branch into `main`, then fast-forward the cursor branch to the resulting `main` commit so both refs point to the same tip.

### Build

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --target WestCoastDrumSynth -j$(nproc)
```

Output: `build/VST3/Debug/WestCoastDrumSynth.vst3/Contents/x86_64-linux/WestCoastDrumSynth.so`

### Testing

Run the Steinberg VST3 validator (build it first, then run against the plugin bundle):

```bash
cmake --build build --target validator -j$(nproc)
build/bin/Debug/validator build/VST3/Debug/WestCoastDrumSynth.vst3
```

All 47 test cases should pass. There are no unit tests or other test frameworks in this repo.

### Linting

No dedicated linter configuration exists. Use standard C++ static analysis if needed (e.g. `clang-tidy`).

### Important caveats

- The VST3 SDK is auto-fetched via CMake FetchContent on first configure (~30s). No git submodule init needed.
- The SDK's sample `editorhost` and Carla cannot render this plugin's VSTGUI-based GUI because they don't initialize the VSTGUI platform factory. The plugin itself is correct; GUI testing requires a full DAW host (Reaper, Bitwig, Ardour, etc.).
- System dependencies required on Linux (installed via apt): `libxcb-util-dev libxcb-cursor-dev libxcb-keysyms1-dev libxcb-xkb-dev libxkbcommon-dev libxkbcommon-x11-dev libcairo2-dev libgtkmm-3.0-dev libstdc++-14-dev`.
- First CMake configure is slow (~40s) due to SDK download; subsequent configures are fast.
