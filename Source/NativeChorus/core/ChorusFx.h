#pragma once
#include <array>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace skelly::fruitychorusfx
{
constexpr int params = 12;
using State = std::array<int, params>; // raw controls, all from 0
constexpr State defaults { 512, 2250, 341, 1500, 2500, 3500, 23, 0, 0, 0, 2500, 0 };
constexpr State rawMax { 1024, 5000, 1024, 5000, 5000, 5000, 1024, 1024, 1024, 100, 5000, 1024 };

// FL's saved state: i32 version (FL writes 1; a version above 1 is not read), then twelve i32 in control order
std::optional<State> parse (const std::vector<std::uint8_t>& state);
void toParams (const State& s, float* params) noexcept;
State fromParams (const float* params) noexcept;
const char* paramName (int param) noexcept;
std::string paramText (int param, float value01);

// The plugin object: FL's control changes (their sample-rate dependent values use the rate set when the control is
// set), the "enabled" reset and the render (interleaved stereo).
class Engine
{
public:
    Engine();
    ~Engine();
    void setRate (int sampleRate) noexcept; // takes effect for the controls set afterwards (as in FL)
    void setParam (int index, int raw) noexcept;
    void load (const State& s) noexcept; // every control in order, like FL's state load
    // FL's "enabled" notification: filter states and delay lines cleared; FL draws the three LFO start phases at
    // random (0…32766 of a 2^24 cycle), skelly takes the given ones
    void reset (std::uint32_t phase1 = 0, std::uint32_t phase2 = 0, std::uint32_t phase3 = 0) noexcept;
    int param (int index) const noexcept;
    void render (const float* in, float* out, int n) noexcept; // interleaved stereo; in may equal out

private:
    struct Impl;
    std::unique_ptr<Impl> d;
};

} // namespace skelly::fruitychorusfx
