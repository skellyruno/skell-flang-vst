# Skelly-owned raw Flangus engine — integration trial

Owner states that their team authored the supplied DSP and requests Skelly Licensing. The license terms have not yet been written. Copyright retained by respective Skelly rights holders; no new redistribution permission is granted by this notice. This does not relicense JUCE, SkellCore or any other dependency. The original supplied DSP and private adapter are intentionally not published in this public repository pending license/distribution decisions.

Use the private source package directory containing Flangus.cpp, SkellFlangusAdapter.h and core/FlangusFastDist.h:
```sh
cmake -S . -B build-raw -DSKELLY_FLANGUS_SOURCE_DIR=/path/to/SkellyFlangus -DJUCE_PATH=/path/to/JUCE
cmake --build build-raw --config Release --target SkellFlang_VST3
cmake -S tests -B build-dsp -DSKELLY_FLANGUS_SOURCE_DIR=/path/to/SkellyFlangus -DCMAKE_BUILD_TYPE=Release
cmake --build build-dsp
ctest --test-dir build-dsp --output-on-failure
```
The standard configuration has no raw-mode changes. Private trial build adds one default-off enable and eight separate integer host controls, raw order 1..9, depth/speed 0..1024, delay/spread 0..128, cross/dry/wet -128..128. Existing IDs, factory presets, plugin identity and legacy mode values are unchanged. Settings menu toggles exclusive raw path and resets raw controls; raw values are edited via host automation/parameter list, not the rack faders. Raw dry/wet is computed once by the supplied engine. Power and output remain common; existing rack output safety guard still applies. Crossfade takes 20ms. Factory selection returns to legacy. Missing raw state fields default to legacy and raw defaults. Unsupported rates fall back to legacy; raw validated 32–192k integer rates. Do not load a raw-mode session in the normal non-raw build expecting the same sound. Dedicated editor controls, FST importer and actual host state roundtrip still needed.

Local portable path tests passed GCC and Clang C++17 Release: five rates and five block sizes, exact frozen output against supplied adapter, disabled legacy passthrough, mono and mode transitions. Private adapter tests passed GCC, Clang, ASan/UBSan: exhaustive raw value roundtrip, five rates, frozen block parity and invalid/finite checks. Live gain automation is block-size dependent (64/512 test max difference .0209675282); do not claim FL equivalence for automation or change original gain ramps silently. Windows MSVC long-double fallback differs from x87, independent parity needs measurements. No FL Studio reference null-test certificate or plugin integration build yet. The JUCE download/local clone failed DNS; existing CI will test standard build, not private source. No screenshot or new VST3 claimed. No source redistribution, private vocals or private FST published.

Original engine code provided by owner, isolated from unrelated LYNX mixer wrapper; namespace corrected; exact raw control records recovered privately for UP and Myers. Speakerz isolated stage is promising but mixer routing fit is inferred, not original saved parameters. Keep all protected preset sounds. Do not replace current Flangus mode until owner A/B, host state/automation, pluginval, Windows VST3, CPU/latency and FL Studio tests pass.
