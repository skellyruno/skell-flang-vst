# Skell-Flang — BEND THE BONES

Original Skell Audio JUCE 8 / C++17 VST3-only modulation-delay plugin, based on the shared SkellCore template. **This is a working development build, not a validated commercial release.**

## Six modes
- **CHORUS:** modulated 1–8 voice delays, feedback, and stereo spread.
- **FLANGER:** short comb delays, bipolar feedback and manual sweep.
- **VOWEL:** comb delay into five interpolated approximate resonant formants.
- **REVERSE:** overlapping reversed Hann-windowed grains with an up-to-1.5-second ring buffer.
- **ORBIT:** delay/panning-based stereo circular motion with air attenuation.
- **TUNED:** pitch- or key-following harmonic comb with monophonic control-path autocorrelation.

## Interface

Screenshot-inspired seven vertical lanes (DEPTH, SPEED, DELAY, SPREAD, CROSS, DRY, WET), eight-step ORD, six tabs, live waveform scope, four mode knobs, FEEDBACK/TONE/OUTPUT, morph target, preset menu, and dark/gunmetal neon Skell look. The default compact editor is 540 × 338 with CORE / COLOR / MOTION pages; scale can be changed to 720, 900, or 1080 pixels wide and is stored with the plugin state. The UI uses original vector drawing. UI is all-vector, so no commercial raster art or fonts are needed.

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


## SkellWorld (opt-in pilot)

Skell-Flang can discover other SkellWorld plugins through the shared SkellCore WorldLink. Run `python tools/skellworld_hub.py` from the **SkellCore template checkout** to start the local-only developer hub, then choose **SIZE → Join SkellWorld** in the plugin. The size menu also shows current online peers. The plugin remains connected if only its editor is closed, until Leave or the instance is unloaded.

Connection is **off by default**; no network code runs on the audio callback, and no parameter IDs, saved preset schemas, DSP algorithms or VST3 identity changed. The hub carries only informational presence/events, not remote parameter control. This integration is a trial PR and needs a Windows VST3 host smoke test before release.
