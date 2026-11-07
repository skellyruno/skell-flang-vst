# Status — Skell-Flang v0.2

- VST3 + standalone CMake target: staged for Windows CI.
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
