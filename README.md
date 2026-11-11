# Skell-Flang — BEND THE BONES

Original Skell Audio JUCE 8 / C++17 VST3-only modulation-delay plugin, based on the shared SkellCore template. **This is a working development build, not a validated commercial release.**

## Five current effects (v0.7)

- **CHORUS** — newly voiced multi-voice stereo detuning with low-speed motion and complementary left/right waveforms.
- **FLANGER** — revised short-delay comb sweep with independent stereo phase and bipolar feedback.
- **FLANGUS** — new original eight-voice-capable ensemble blending slow chorus movement and faster comb travel, influenced by the stacked-modulated-delay concept (not a replica or use of FL Studio code).
- **ORBIT** — existing rotating stereo/spatial processor preserved by request.
- **DIMENSION** — new stereo doubling/micro-delay width engine for vocals and instruments.

For old FL Studio sessions, **Vowel (mode 2), Reverse (3) and Tuned (5)** retain their original internal IDs and DSP. They are no longer offered in the current selector/preset browser; the new engines are **Flangus (6)** and **Dimension (7)**. Old automation IDs, plugin manufacturer/product IDs, and legacy parameter values are unchanged.

## Interface (v0.7)

The compact 540 × 338 editor has five mode tabs, seven always-visible main faders, a bottom row of seven editable rotary tone/character controls, and a live audio monitor. The previous CORE/COLOR/MOTION bottom navigation is removed. The **MOD** button opens an integrated destination/morph/LFO drawer with collision-safe layout; **BACK** returns to the tonal controls. Clicking the vector skull cycles accent skins. The top-right gear opens window sizes and the automatable Effect enabled toggle; there is no ACTIVE/BYPASS banner. The preset browser is grouped by sound family and uses custom Skell-themed popup painting. All four sizes (540, 720, 900, 1080 wide) are retained.

The 16 factory presets now load complete mix, filter, feedback, depth, spread, modulation and character settings. Controls, ranges and effect choices remain automatable; selecting a factory preset resets morph to zero so old morph settings do not make sounds unexpectedly different.

## Build Windows

```powershell
cmake -S . -B build -A x64
cmake --build build --config Release --parallel 2
```

CMake fetches pinned JUCE 8.0.12 if JUCE_PATH is not supplied and links the **local SkellCore/** included by the repository. To use a local JUCE checkout pass `-DJUCE_PATH=C:/path/to/JUCE`. The main target is **SkellFlang**, with unique VST3 plugin code `Fl26` and bundle ID `audio.skell.skellflang`.

GitHub Actions **Build Skell-Flang Windows VST3 Only** builds and packages only the VST3 bundle. There is no standalone executable target or standalone EXE in the artifact. Portable and sanitized DSP test jobs are required alongside the Windows compile. The built plugin is usually under `build/SkellFlang_artefacts/Release/VST3/`.

## Real-time operation (v0.4)

- VST3-only; audio runs in the host audio callback. No separate executable.
- Zero reported host PDC and no look-ahead. The musical wet effects retain their inherent delays, and pitch tracking still requires a brief analysis history.
- Main DSP controls and four mode macros use sample-rate-independent 4 ms smoothing updated each sample. Targets arrive at audio-block boundaries; this does not imply sample-accurate VST3 automation events.
- Mix, crossfeed and output use 6 ms ramps; power uses 5 ms and morph uses 25 ms. Fully bypassed input no longer goes through the active output clamp.
- Tuned pitch-following now uses antialiased decimation and updates its analysis roughly every 10-11 ms at common sample rates, without buffering output audio.
- Waveform scope displays 64 ordered samples representing approximately 43 ms at any sample rate. Peak decay and CPU indication are independent of host audio buffer size.
- Delay buffers allocate in prepareToPlay. Our per-sample DSP uses fixed state with no explicit locks, file I/O or heap allocations.
- Real FL Studio listening, pluginval, and CPU profiling at 32/64/128 samples per block are still required.
## v0.5 polishing and CPU work

- **Windows VST3-only** remains the sole build target; no standalone app or EXE.
- **Mix headroom fixed:** the previous processed-output hard clamp at ±1.0 is removed. The output retains normal floating-point DAW headroom above 0 dBFS, with a conservative ±16 guard against abnormal, non-finite or runaway values; fully bypassed finite input passes through unchanged.
- **Reverse-grain CPU optimization:** each EngineBank builds a 2,049-point Hann window during prepare, then interpolates it when playing grains. This removes repeated `cos()` calls in the reverse audio loop without changing its allocation model.
- **Tuned-mode CPU optimization:** key frequencies are cached until the selected root or scale changes; voice detune ratios are multiplied iteratively rather than recomputing `pow()` for each voice on every sample. A small spread threshold avoids redundant ratio updates.
- **Compact UI:** unified power and host-sync buttons, shorter panel titles, responsive mode and preset selections, actual dark popup-menu styling, and saved host-dragged window width.
- **Tests:** added above-0 dBFS headroom and mid-stream tuned-key-change checks. The new builds must still be validated in FL Studio, including mode changes, overs and dense morphs, before commercial release.

## v0.6 validation and stereo fixes

- **Synced modulation:** the SPEED fader selects tempo subdivisions independently of the selected sine/triangle/saw/square LFO waveform. This fixes a coupling where selecting an LFO shape unexpectedly changed its speed.
- **Real-time paths:** interpolated delay taps avoid integer division on every sample, and 257-point equal-power lookup gains remove repeated audio-thread trigonometry when morphing or switching modes.
- **ORBIT stereo:** both channels now retain their own signal rather than being summed to mono before processing. This prevents phase-opposed stereo signals from cancelling into silence.
- **Host VST3 compatibility:** the default program slot has a non-empty name for Steinberg's validator. The factory preset browser remains independent of the single host program.
- **Continuous integration:** a portable eight-voice/morph DSP performance benchmark reports single-core-equivalent timings; Windows additionally runs a real JUCE editor/render/layout smoke check at four sizes and Tracktion pluginval host compatibility tests. CI-only test executables are never included in the Windows VST3 artifact.
- **Compiler hygiene:** GCC/Clang portable test targets treat warnings as errors; GitHub Actions checkout/artifact steps use Node 24-compatible versions.
- **Still required for commercial release:** FL Studio loading/listening and real CPU/latency measurements on multiple Windows machines, long automation stress, and independent true through-zero DSP design.

## DSP behavior and limitations

The delay-line Hermite interpolation and feedback filtering are bounded and allocated in `prepareToPlay`. Modulated voice delay times now use a 3.5 ms slew filter and feedback automation uses an independent 8 ms filter, calculated per sample without heap allocation. Host PDC is fixed at **0 samples**, avoiding an artificial misalignment of the dry and wet flanging signals. The reverse-grain effect has natural creative time offset but no extra host-declared delay.

The chorus DETUNE/WIDTH/WARMTH/ENSEMBLE and orbit RADIUS/TILT/AIR/SPIN controls now drive real processing changes. Tuned TRACK/GLIDE/HARMONIC/RESONANCE smooth and shape harmonic combs. Reverse SHIMMER blends a faster backwards grain read; it is a creative approximation, not a pitch-preserving commercial octave shifter. True through-zero flanging has not been implemented; the formerly misleading THRU ZERO* macro is now called SWEEP RANGE and changes the actual sweep travel.\n\nThe vowel table is approximate original design data, not validated measured phonetics. ORBIT is an original perceptual ITD/pan approximation, not a measured HRTF. TUNED is a prototype monophonic tracking mode: it does not yet offer a certified polyphonic note detector. Sliders marked `*` are prototype controls whose full specified behavior is pending. Advanced oversampling, A/B preset snapshots, editable vowel paths, exact through-zero alignment, pluginval, DAW regression runs, and professional listening validation are not yet delivered.

Run portable DSP tests without JUCE:
```sh
cmake -S tests -B build-dsp -DCMAKE_BUILD_TYPE=Release
cmake --build build-dsp
ctest --test-dir build-dsp --output-on-failure
```

## Distribution

JUCE and the shared SkellCore source have separate licensing/provenance obligations. Review them before any commercial release. This repository preserves the SkellCore files and changes only the quickstart plugin entry points.

## Packaging verification

The Windows CI now parses and normalizes the JUCE-generated `moduleinfo.json` (including trailing commas) before distribution, then uploads `dist/` directly as an Actions artifact containing **only the VST3 bundle**. The portable DSP test suite also runs with AddressSanitizer and UndefinedBehaviorSanitizer. This is packaging and static/runtime test coverage, not a substitute for pluginval or FL Studio host validation.

## v0.7 verification notes

The shared SkellCore renderer generates 40 real-editor screenshot scenes (5 effect modes × 2 drawer states × 4 editor sizes), checks visibility and bounds, and verifies mode and preset interactions. Portable DSP tests exercise all eight internal engine indices (five current plus three compatibility modes) across sustained signals and stress paths. The Windows workflow compiles only an x64 VST3 and performs pluginval. A successful CI build confirms structural behavior, **not** subjective mixing quality: FL Studio listening, preset comparisons, low-end machine CPU tests and 24-hour automation stress remain to do.
