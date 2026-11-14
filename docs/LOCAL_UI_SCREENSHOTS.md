# Skell Flang — LOCAL REAL JUCE SCREENSHOTS 🩻

This repository participates in SkellCore's four-scene local UI QA. **Don't generate a replacement UI image or present an audio waveform as a screenshot.** Capture the actual C++ `PluginEditor` instantiated with the real processor and component hierarchy.

- CMake option: `-DSKELLFLANG_BUILD_UI_SMOKE=ON`
- Development renderer target: `SkellFlangUiSmoke`
- Default: exactly **four** actual states: `Skell-Flang-720-chorus-unmorphed, Skell-Flang-720-flanger-unmorphed, Skell-Flang-720-chorus-morphed, Skell-Flang-720-chorus-graphite`
- `--full`: opt-in previous comprehensive UI regression
- Special case: real effect/morph and graphite skin
- Current verification: ****VERIFIED** real JUCE Linux native compile + actual 4/4 Quick4 render (2.77 seconds under Xvfb; build time separate)**. A committed renderer is not proof of a successful build on the latest revision.

Common entrypoint: `skellcore-vst-template/tools/quick4_projects.py --plugin skell-flang-vst --project /path/to/skell-flang-vst --juce /path/to/JUCE --skellcore /path/to/skellcore-vst-template --output /tmp/skell-flang-vst-quick4`.

On restricted machines, source pinned dependencies must already exist; never quietly substitute different live plugin code or claim the screenshot is current if source compiling fails.

GitHub Actions is not needed for ordinary screenshots; any existing screenshot-only workflow is **manual-only**. Maintain VST3, DSP/latency/CPU, Windows, and FL Studio validation separately.
