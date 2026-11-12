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

## v0.8 — November 2026

- Removed MOD drawer and its top button; integrated target/morph/wave/sync controls permanently below tone knobs.
- Replaced signal panel L/R bars with a Skell Key-derived 52-segment live header meter, using real peak readings and UI-thread smoothing.
- Mode-specific identity palette, consistent selected tab feedback, smoother button surfaces, branded dropdown treatment.
- Fixed Windows menu text mojibake by using ASCII punctuation in all popup captions; tightened custom popup styling and sizes.
- Chorus ENSEMBLE control now drives an independent slow motion; Flangus comb delay floors at 1.2 ms.
- Preserved Orbit DSP, existing preset and parameter IDs, default window size, zero PDC and Windows VST3-only packaging.
- Run real editor render, sanitizer, portable DSP, and pluginval in CI; test listening in FL Studio before release.


## v0.9 — menu, fader and theme separation overhaul

- **Menu regression repair:** JUCE popup windows are separate top-level components in a host; the preset and settings menus now call `menu.setLookAndFeel(&skin)` explicitly and each preset submenu gets the same look-and-feel. The original custom popup drawing finally applies in FL Studio instead of the generic host/default appearance. Menu font, background, separators, tick marks, highlighted rows and selected accent follow Skell-Flang's own theme. Captions are ASCII-safe.
- **Less crowded editor:** new aspect 720 x 500. Saved width presets remain 540/720/900/1080; respective heights are 375/500/625/750. The control deck was re-spaced vertically without relocating the familiar faders, tone knobs, morph row, scope or header. User resize maintains the fixed new aspect.
- **Precision hardware faders:** replaced broad black vertical wells and oversized horizontal caps with narrow machined tracks, restrained side ticks, slim metal caps and calibrated active fill. Knob/character strip remains below.
- **Selected effect:** removed all redundant underlines; a selected mode now illuminates its own button surface, frame and subtle chrome edge, keeping the other four neutral.
- **Skull cycles true interface themes** (Midnight, Graphite, Bone) independently of per-effect accent identities: Chorus seafoam; Flanger cyan; Flangus neon lime; Orbit periwinkle; Dimension warm gold. Clicking the skull changes the panel, header, text, dropdowns, knobs, faders and popup surfaces; the effect identity color never changes.
- Expanded the shared SkellCore visual suite to **50 real JUCE editor screenshots**: 5 modes x 2 morph settings x 4 sizes, plus all five modes in Graphite and Bone at 720 px. It now checks true theme changes and constant effect identity color, in addition to independent control hit areas.
- **No DSP/automation rework:** plugin IDs, parameters, factory presets, zero host PDC and Orbit sound all preserved. This is a UI-only pass.

## v0.9.1 — SkellWorld opt-in presence and realtime release gates

The old draft SkellWorld PR was ported on top of the current v0.9 UI/DSP without downgrading the bundled SkellCore UI renderer. Two small, local-only protocol/worker headers are vendored from skellyruno/skellcore-vst-template, pinned at commit 214badd41c05bfcc7d7528dba5b5984a6d607cc3, so there is no new remote dependency. The themed gear menu now contains a **SKELLWORLD** section with opt-in Join/Leave and read-only connection status. The WorldLink is processor-owned and never touched by processBlock(). No automation, latency or VST3 identity changed.

Release tests now include portable SW1 protocol rejection/mailbox tests and real-time C++ allocation checks for eight internal modes plus morph, and a Windows native-editor smoke test of join/leave with no hub. A live two-instance Windows FL Studio hub round-trip and long listening stress still require actual in-host testing. Distribution remains **Windows VST3 only**.

- v0.9.1 host-native UI smoke extends to 44.1/48/96/192 kHz, mono/stereo buffers of 1/17/64/257/512 samples, mode/feedback/morph automation, session XML save/reload and exact output transparency after bypass ramp. See Windows CI, distinct from real FL Studio acceptance.


## v0.9.2 — cleaner hardware UI, four active dials

- Retains the existing rack layout, effect modes and SkellWorld setting; refines the buttons and replaces the seven simultaneous bottom rotary controls with **three always-visible essentials (FEEDBACK, TONE, OUTPUT)** plus **one CHARACTER dial**, controlled by four compact selectable macro labels. The four original extra0–extra3 rotary controls and APVTS attachments remain alive and unchanged, just hidden when unselected: no parameter IDs, automation, preset values or DSP algorithms were removed.
- Active macro is clear, and character labels update automatically as the user selects Chorus / Flanger / Flangus / Orbit / Dimension. Clicking the skull still cycles whole-interface themes while leaving effect identity and the selected character macro unchanged. Host-mode changes default to macro 1, intentionally.
- Main vertical console faders now use a single narrow rail, six unobtrusive calibration marks, compact brushed-metal thumbs, visible percentage/value readouts, and softer layering. The top skull, preset and settings buttons share a uniform machined-face treatment; preset selector is left-aligned with its own caption and larger selected preset text.
- Kept the separate, branded popup-menu styling and all Windows/SkellCore/SkellWorld behaviors.
- SkellCore native editor-render suite expanded to **70 screenshots** covering 5 effects x 4 sizes x 2 morph states, 5 effects x two alternate full themes and 5 effects x four character selectors. Added exact visibility, no-overlap, click action and skull-vs-macro tests.
- UI-only patch, so Orbit and other DSP mode output are unchanged; physical FL Studio testing remains the acceptance gate.
