# Uploaded voice-modulator code review for Skell-Flang v0.7.1

Source provided by the project owner: `gfhhgfh`, a ZIP containing a Linux/PipeWire/Qt voice-effects app with ChorusEffect, FlangerEffect, EffectPipeline, and a Modulator using ping-pong curves.

## What was adapted

- The supplied chorus/flanger effect delay buffers store an unprocessed feed-forward input and only mix feedback into the flanger delay. Skell-Flang previously applied `tanh` to the **entire** delay-line input, even when feedback was zero. v0.7.1 uses a clean dry signal plus soft-protected **feedback only** for Chorus, Flanger, Flangus and Dimension.
- The supplied `Modulator` offers an `EaseInOut` smoothstep and ping-pong timeline. v0.7.1 adapts that technique for the existing Triangle LFO option (same timing and parameter index, smoother reversals).
- The returned wet mix for the four current non-Orbit delay modes now uses a transparent low-level path and a soft knee for exceptional peaks, rather than saturating all amplitudes.

## Deliberately not imported

- PipeWire audio-device, Qt user-interface and source-level threading architecture: not suitable for a Windows VST3 built using JUCE.
- Linux app's mono 50 ms / 15 ms linear-interpolation delay buffers: Skell-Flang already has stereo and higher-quality cubic interpolation with preallocation.
- Audio-thread `std::mutex` in `EffectPipeline`: unsafe to import into a realtime plugin callback.
- Third-party JSON/profile dependencies: unnecessary for the existing VST3 parameters and state handling.

## Compatibility and verification

No old parameter ID, plugin bundle ID, output latency, mode ID, or preset index was changed. The legacy Vowel/Reverse/Tuned effects and the approved Orbit sound preserve their existing algorithms. Portable DSP tests check the new soft knee, smooth LFO continuity, and all existing stress cases; GitHub Actions performs the Windows VST3 build, JUCE renderer and pluginval validation.

This change is an engineering adaptation of authorized source, **not** a bit-identical reimplementation of the uploaded app. Listening validation in FL Studio remains necessary.
