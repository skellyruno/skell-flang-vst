# Status — Skell-Flang v0.3

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
