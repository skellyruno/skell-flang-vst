# Skell-Flang — BEND THE BONES

Original Skell Audio JUCE 8 / C++17 VST3 and standalone modulation-delay plugin, based on the shared SkellCore template. **This is a working development build, not a validated commercial release.**

## Six modes
- **CHORUS:** modulated 1–8 voice delays, feedback, and stereo spread.
- **FLANGER:** short comb delays, bipolar feedback and manual sweep.
- **VOWEL:** comb delay into five interpolated approximate resonant formants.
- **REVERSE:** overlapping reversed Hann-windowed grains with an up-to-1.5-second ring buffer.
- **ORBIT:** delay/panning-based stereo circular motion with air attenuation.
- **TUNED:** pitch- or key-following harmonic comb with monophonic control-path autocorrelation.

## Interface

Screenshot-inspired seven vertical lanes (DEPTH, SPEED, DELAY, SPREAD, CROSS, DRY, WET), eight-step ORD, six tabs, live waveform scope, four mode knobs, FEEDBACK/TONE/OUTPUT, morph target, preset menu, and dark/gunmetal neon Skell look. Fixed-aspect resizing supports 75%, 100%, 125% and 150% sizes. UI is all-vector, so no commercial raster art or fonts are needed.

## Build Windows

```powershell
cmake -S . -B build -A x64
cmake --build build --config Release --parallel 2
```

CMake fetches pinned JUCE 8.0.12 if JUCE_PATH is not supplied and links the **local SkellCore/** included by the repository. To use a local JUCE checkout pass `-DJUCE_PATH=C:/path/to/JUCE`. The main target is **SkellFlang**, with unique VST3 plugin code `Fl26` and bundle ID `audio.skell.skellflang`.

GitHub Actions **Build Skell-Flang Windows VST3 + Standalone** builds both plugin types on Windows and uploads a ZIP when checks pass. A separate Linux job exercises portable DSP. The Windows VST3 directory is usually under `build/SkellFlang_artefacts/Release/VST3/`; the standalone executable is usually under `build/SkellFlang_artefacts/Release/Standalone/`.

## DSP behavior and limitations

The delay-line Hermite interpolation and feedback filtering are bounded and allocated in `prepareToPlay`. Host PDC is fixed at **0 samples**, avoiding an artificial misalignment of the dry and wet flanging signals. The reverse-grain effect has natural creative time offset but no extra host-declared delay.

The chorus DETUNE/WIDTH/WARMTH/ENSEMBLE and orbit RADIUS/TILT/AIR/SPIN controls now drive real processing changes. Tuned TRACK/GLIDE/HARMONIC/RESONANCE smooth and shape harmonic combs. Reverse SHIMMER blends a faster backwards grain read; it is a creative approximation, not a pitch-preserving commercial octave shifter. Through-zero flanging remains a marked prototype.\n\nThe vowel table is approximate original design data, not validated measured phonetics. ORBIT is an original perceptual ITD/pan approximation, not a measured HRTF. TUNED is a prototype monophonic tracking mode: it does not yet offer a certified polyphonic note detector. Sliders marked `*` are prototype controls whose full specified behavior is pending. Advanced oversampling, A/B preset snapshots, editable vowel paths, exact through-zero alignment, pluginval, DAW regression runs, and professional listening validation are not yet delivered.

Run portable DSP tests without JUCE:
```sh
cmake -S tests -B build-dsp -DCMAKE_BUILD_TYPE=Release
cmake --build build-dsp
ctest --test-dir build-dsp --output-on-failure
```

## Distribution

JUCE and the shared SkellCore source have separate licensing/provenance obligations. Review them before any commercial release. This repository preserves the SkellCore files and changes only the quickstart plugin entry points.

## Packaging verification

The Windows CI now parses and normalizes the JUCE-generated `moduleinfo.json` (including trailing commas) before distribution, then uploads `dist/` directly as an Actions artifact so users download a single ZIP containing the VST3 bundle and standalone executable. The portable DSP test suite also runs with AddressSanitizer and UndefinedBehaviorSanitizer. This is packaging and static/runtime test coverage, not a substitute for pluginval or FL Studio host validation.
