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
