# Enhanced Motion trial
Enable Settings > Enhanced Motion / Orbit (A/B) after choosing any existing factory sound. Default OFF. Existing factory sounds and state without the appended enhancedMotion ID remain legacy. The toggle is smoothed over 20 ms. No new program identity, no mode renumbering, no replacement of the private native Flangus algorithm.
- Chorus / legacy Flangus / Dimension: maintain separate continuous fractional-rate phase to prevent wrap discontinuities (inspired by the native Flangus motion topology).
- Flanger: 8 ms attack / 80 ms release follower replaces audio-rate rectified delay modulation at full enhanced setting.
- Orbit: clustered moving voices, constant-power pan, depth-dependent interaural timing, distance movement, far-ear/rear shading, less input saturation. Not a measured HRTF or a binaural renderer. Existing ellipse, radius, rate, air and spread continue to affect the motion.
- Native exclusive Flangus remains unchanged; the new option is for the rack modes only.
Portable test covers five rates, maximum-order orbit movement, anti-phase preservation, depth sensitivity, and all five modes during quality automation. Listening and performance approval remain open. No private source or audio included.

## Listening candidate revision 4 (real-source measurements)
Tighter voice cluster (.16 cycles) and finite-slope depth mapping 1-(1-depth)^4. Tested the five-visible-mode extraction of 691e22a (normalized FNV-1a 3908922119), with only the two stated Orbit constants changed. Settled single-layer rack mix, original factory controls, 44.1 kHz stereo, same eight-second excerpt per source, 100ms active-window normalized L/R energy-difference standard deviation. This is not a full host render or a perceptual listening verdict.

| Source | Preset | Legacy | First enhanced | Rev4 |
|---|---|---:|---:|---:|
| Hop Out | Halo | .135 | .138 | .253 |
| Hop Out | Carousel | .110 | .161 | .251 |
| Hop Out | Pulse | .188 | .072 | .172 |
| Speakerz | Halo | .176 | .134 | .287 |
| Speakerz | Carousel | .103 | .198 | .346 |
| Speakerz | Pulse | .174 | .062 | .161 |

All 6 rev4 comparisons improved over the first candidate on this movement metric; Pulse remains less mobile than legacy on these clips. No sample clipping in any rev4 clip. Measured solo DSP/mixer 1.51–3.34% real-time budget in Linux, compared to legacy 1.20–2.30%; no complete plugin or Windows CPU claim. Keep legacy default and audition level-matched. Source audio remains private.
