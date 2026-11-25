// Copyright retained by the Skelly team and respective rights holders.
// Skelly Licensing: provisional ownership notice; definitive terms pending.
// No additional redistribution permission is granted by this notice.
#pragma once
// Flangus (EffectKind::fruityFlangus): [0] order 1…9, [1] depth 0…1024, [2] speed 0…1024, [3] delay 0…128,
//   [4] spread 0…128, [5] cross −128…128, [6] dry −128…128, [7] wet −128…128.

#include <array>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>


namespace skell::flangusfx
{
constexpr int params = 8;
constexpr int maxOrder = 9;
using State = std::array<int, params>; // order, depth, speed, delay, spread, cross, dry, wet (raw)
constexpr State defaults { 4, 512, 768, 8, 128, -32, 96, 96 };
constexpr std::array<int, params> rawMin { 1, 0, 0, 0, 0, -128, -128, -128 };
constexpr std::array<int, params> rawMax { 9, 1024, 1024, 128, 128, 128, 128, 128 };

// FL's saved state: eight i32 in control order, no version
std::optional<State> parse (const std::vector<std::uint8_t>& state);
void toParams (const State& s, float* params) noexcept;
State fromParams (const float* params) noexcept;
const char* paramName (int param) noexcept;
std::string paramText (int param, float value01);

// The plugin object: FL's control changes, sample rate, block size, flush, and the render (interleaved stereo).
class Engine
{
public:
    Engine();
    ~Engine();
    // prepares the delay line for this rate (allocates; call off the audio thread) and applies it
    void prepare (int sampleRate);
    // FL's ProcessParam: a change of controls 0…5 re-runs the voice set-up (which may clear the delay line)
    void setParam (int index, int raw) noexcept;
    void load (const State& s) noexcept; // every control in order, like FL's state load
    void setRate (int sampleRate) noexcept;
    void setBlockSize (int frames) noexcept;
    void setDeterministicGainRamp (bool enabled) noexcept; // opt-in sample-rate slew; original is default
    void flush() noexcept; // FL's flush: gains at their targets, LFO phases restarted, delay line cleared
    int param (int index) const noexcept;
    // n ≤ 4096 frames, interleaved; in and out may be the same buffer
    void render (const float* in, float* out, int n) noexcept;

private:
    struct Impl;
    std::unique_ptr<Impl> d;
};

}
