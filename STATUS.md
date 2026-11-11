# Status — Skell-Flang v0.6

- VST3-only CMake target and GitHub packaging; standalone removed by request.
- DSP: six prototypes, bounded ring buffers, Hermite fractional reads, feedback saturation; improved mode-specific knob mapping, tuned glide, and shimmer.
- Performance: 80 ms per modulated voice, 1.5 seconds only for reverse; tracking only in tuned paths.
- Editor: original vector layout, neon green with per-mode accent, six tabs, faders, knobs, live scope and resizable window.
- Host automation + XML state: implemented for main controls.
- Presets: 19 named factory starting points.
- Latency: 0 samples of declared PDC, no dry-only artificial delay.
- Portable DSP tests: available in tests/.
- First MSVC compile reported two popup-callback capture errors; patched for revalidation. Successful Windows VST3/EXE artifact, actual FL Studio load, pluginval, audio benchmarks and listening remain pending.
- Pending: proper through-zero, oversampling, per-mode extra parameter banks, A/B, MIDI learn, UI accessibility polish, controlled anti-zipper smoothing on all automation, calibrated response display.

- v0.2 packaging follow-up: Windows CI validates/normalizes JSON module info, uploads one ZIP directly, adds sanitizer job. New workflow result pending.

## v0.3 changes
- Compact editor defaults to 540 × 338 (resizable), with uncluttered CORE / COLOR / MOTION pages, distinct rib emblem and restrained per-mode underline.
- Rescaled label/button/dropdown typography, dark popup/combo styling, remembered UI width.
- FLANGER first mode control renamed to SWEEP RANGE and wired to the actual modulation travel. No claim of through-zero support.
- Per-voice/per-channel sample-rate-aware 3.5 ms fractional-delay slew plus 8 ms feedback automation smoothing, without per-block allocation.
- VST3 x86_64-win module binary validated in CI; artifacts no longer include an EXE.
- New abrupt parameter automation stress and functional sweep-range regression tests.
- Remaining: true through-zero mode with constant PDC contract, improved formant/vowel models, real host pluginval/FL Studio listening and performance.
- Pitch tracker now selects a sample-rate-dependent input decimation (about 12 kHz) to retain low-note tracking at 96/192 kHz; regression tests cover 82 Hz and 220 Hz.

## v0.4 realtime and compact UI pass
- No standalone: Windows VST3-only target maintained.
- Sample-rate-aware 4 ms per-sample control/macro smoothing, with 5-25 ms mix, power and morph ramps.
- Pitch follower: one-pole antialiased downsampling and ~10.7 ms hop for faster note decisions, without added host latency.
- State-derived startup gains prevent load glitches; completely bypassed input preserves samples above 0 dBFS.
- Live waveform is displayed in chronological order with a ~43 ms window across sample rates.
- Peak meter decay and CPU load display follow real elapsed audio time, not varying host callback block counts.
- Dynamic GUI value tooltips, readable realtime footer, stable compact 540 x 338 editor.
- Synchronized LFO rates avoid per-sample power calculations; multiple older CI builds are canceled when superseded.
- Remaining: FL Studio realtime stress at 32-128 sample blocks, pluginval, actual measured CPU and audio listening tests.

## v0.5 — compact UI + audio-path efficiency
- Only Windows VST3 produced; JUCE/CMake plugin IDs unchanged.
- Replaced active-output ±1.0 hard clipping with float headroom and exceptional ±16 safety clamp; full bypass retains finite input.
- Reverse granular processing now interpolates a precomputed 2049-sample Hann table, avoiding per-grain audio-thread trigonometry.
- TUNED caches root/scale key frequencies and computes iterative detune ratios, reducing expensive per-sample operations.
- The panel/preset/mode interactions receive immediate state updates; popup menu colours follow the plugin LookAndFeel.
- Power and tempo sync are themed text toggles; page captions shortened to avoid clipping at 540×338.
- Manual resize width is persisted; factory presets and automation parameter identities are unchanged.
- Portable regression tests include output headroom/non-finite guards and tuning-cache invalidation.
- Pending: pluginval, FL Studio actual load and host-boundary automation testing, latency contract for any future true through-zero mode, CPU profiling with full 8-voice morph and grains.


## v0.6 hardening
- Independent tempo-sync SPEED vs waveform SHAPE controls, with portable tests.
- Fractional delay circular indexing no longer uses integer modulo, with 32k+ wraparound reference test.
- Equal-power 257-point morph/mode LUT, tested for endpoints and near-unity power.
- Anti-phase Orbit stereo signal no longer collapses to silence; dedicated regression.
- Non-empty host program slot fixes Steinberg VST3 validator rejection.
- VST3 GUI host check via pluginval, plus Windows JUCE render/layout/parameter smoke tests for 540/720/900/1080 widths.
- Real 8-voice and simultaneous 8+8-voice morphing performance benchmark (non-gating measured CPU).
- Compiler warning cleanup, portable tests build with -Werror, Node24 GitHub actions.
- Remaining: measured FL Studio experience, real-world listening, true through-zero, 24-hour automation stress and low-spec Windows profiling.

## v0.6.1 — real-editor renderer, sound and visual refinement
- Bundled the shared SkellCore headless JUCE renderer and added a `SkellCore::UIRenderer` CMake target, using the real processor/editor, not a mock.
- Real-editor regression harness now captures 20 screenshots across 540/720/900/1080 widths and CORE/COLOR/MOTION/TUNED views, comparing MORPH minimum and maximum.
- Corrected JUCE's horizontal MORPH thumb/track drawing (the old vertical draw branch mapped X to Y).
- Machined-steel vector knob design, coordinated understated mode colours, explicit L/R meters, skeletal waveform-monitor geometry, and consistent compact text sizing after mode changes.
- TUNED pitch input uses a per-block energy/hysteresis mid/side selection, avoiding cancellation of inverted stereo input; real editor host test checks a 440 Hz anti-phase tone.
- Cleared stale pitch after silence and made resonator glide time consistent between sample rates.
- Preserved automation IDs, factory presets, plugin identity and zero reported host PDC. Windows VST3 only; the UI smoke executable is used solely in CI, never distributed.
- Revision `d4ca90ef` passed portable DSP tests, ASan/UBSan, Windows VST3 compilation, Steinberg VST3 validation, pluginval and a 20-scene real-editor render test.
- Outstanding: genuine through-zero design with an explicit PDC contract, in-DAW FL Studio automation and listening validation, and low-spec Windows/long-duration CPU profiling. Observational Linux CI CPU numbers must not be confused with FL Studio measurements.

## v0.7 — five-effect refresh

- Primary selectable modes are CHORUS (0), FLANGER (1), FLANGUS (6), ORBIT (4), and DIMENSION (7).
- Legacy VOWEL (2), REVERSE (3), TUNED (5) remain accessible through restored host states and host automation for backward compatibility, but are hidden in the new UI.
- Revised chorus/flanger tonal algorithms and slower exponential free-running LFO speed mapping; independent new FLANGUS and DIMENSION engines.
- Preserved original Orbit stereo DSP.
- Factory sound bank reduced to 16 intentional presets with full parameter edits and morph reset.
- Removed CORE/COLOR/MOTION navigation and the ACTIVE/BYPASS header banner; added always-visible seven-knob tone strip and MOD drawer.
- Added clickable vector skull theme cycling, gear menu for sizes and effect enable, and custom themed grouped preset popup.
- Shared renderer expanded to 40 scene captures across all current modes, drawers and sizes.
- Portable DSP, sanitizers, and Windows VST3 must be verified in current CI; actual FL Studio listening and long host-automation stress remain open.
