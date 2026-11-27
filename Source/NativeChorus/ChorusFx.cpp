#include "core/ChorusFx.h"

//
// Arithmetic follows the plugins operation by operation: float/double steps in their order with one rounding each
// (no contraction), in the SSE rounding and denormal modes each engine runs in.

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <functional>

#if defined(__SSE__) || defined(_M_X64) || defined(__x86_64__) || defined(__aarch64__) || defined(_M_ARM64) // x86 SSE, or ARM64 through core/Simd.h (sse2neon)
#include "core/Simd.h"
#define skelly_CHORUS_MXCSR 1
#if (defined(__GNUC__) || defined(__clang__)) && (defined(__x86_64__) || defined(__i386__))
#include <cpuid.h>
#include "core/Simd.h"
#endif
#endif

#if defined(__clang__)
#pragma clang fp contract(off)
#pragma STDC FENV_ACCESS ON
#elif defined(__GNUC__)
#pragma GCC optimize("fp-contract=off", "rounding-math")
#endif

namespace skelly
{
namespace
{
// SSE mode for the duration of a render: `bits` are or-ed into MXCSR (0x8040 = flush-to-zero + denormals-are-zero,
// 0xE040 = the same with rounding toward zero)
struct CsrMode
{
#ifdef skelly_CHORUS_MXCSR
    unsigned saved;
    explicit CsrMode (unsigned bits) noexcept : saved (_mm_getcsr()) { _mm_setcsr ((saved & ~0x6000u) | bits); }
    ~CsrMode() { _mm_setcsr (saved); }
#else
    explicit CsrMode (unsigned) noexcept {}
#endif
    CsrMode (const CsrMode&) = delete;
    CsrMode& operator= (const CsrMode&) = delete;
};

inline std::int32_t i32At (const std::vector<std::uint8_t>& d, std::size_t o) noexcept
{
    std::int32_t v;
    std::memcpy (&v, d.data() + o, 4);
    return v;
}
inline float norm (int raw, int lo, int hi) noexcept
{
    return std::clamp ((float) (raw - lo) / (float) (hi - lo), 0.0f, 1.0f);
}
inline int rawFrom (float v01, int lo, int hi) noexcept
{
    return lo + (int) std::lround (std::clamp ((double) v01, 0.0, 1.0) * (hi - lo));
}
// FL's control value: (raw − min) · (1 / (max − min)) in double, clamped to 0…1, as float
inline float flValue (int raw, int lo, int hi) noexcept
{
    const double v = (double) (raw - lo) * (1.0 / (double) (hi - lo));
    return (float) std::clamp (v, 0.0, 1.0);
}
inline int truncI (double v) noexcept { return (int) (long long) v; }
// float square root as one SSE instruction, rounded in the current mode (a library sqrtf may not be)
[[maybe_unused]] inline float sqrtF (float v) noexcept
{
#ifdef skelly_CHORUS_MXCSR
    return _mm_cvtss_f32 (_mm_sqrt_ss (_mm_set_ss (v)));
#else
    return std::sqrt (v);
#endif
}
} // namespace

// ================================================================================================= Fruity Chorus (§1)
namespace fruitychorusfx
{
namespace
{
constexpr int lineSize = 8192, lineMask = lineSize - 1;

// the four LFO tables (§1.3): parabola, its cube, the three-partial sine, its cube; 0…32766
struct Tables
{
    std::array<std::array<std::int32_t, 4096>, 4> t {};
    Tables() noexcept
    {
        auto value = [] (float v) { return truncI ((double) ((v + 1.0f) * 16383.0f)); };
        const double norm3 = 3.0 / (std::sqrt (2.0) * 2.0 + 1.0);
        for (int i = 0; i < 4096; ++i)
        {
            const float f = (float) ((double) i / 2048.0);
            const float p = 1.0 > (double) f ? (f * 4.0f) * (1.0f - f) : ((f - 1.0f) * -4.0f) * (2.0f - f);
            t[0][(std::size_t) i] = value (p);
            t[1][(std::size_t) i] = value ((p * p) * p);
            const double fd = f;
            const double s = std::sin (fd * 3.1415) + (std::sin (fd * 9.4245) + std::sin (fd * 6.283)) * 0.3333333333333333;
            const float sf = (float) (s * norm3);
            t[2][(std::size_t) i] = value (sf);
            t[3][(std::size_t) i] = value ((sf * sf) * sf);
        }
    }
};
const Tables& tables()
{
    static const Tables t;
    return t;
}
} // namespace

struct Engine::Impl
{
    const Tables& tab = tables();
    State raw = defaults;
    std::array<float, params> x {};
    int fs = 44100;
    std::uint32_t phase[3] {}, inc[3] {};
    int wave[3] {};
    int stereo = 0, delay = 0, depth = 0;
    bool lpLine = false, wetOnly = false;
    float a = 0.0f, b = 0.0f;
    float st[2] {}, prev[2] {};
    std::array<float, lineSize> line[2] {};
    int w = 0;

    void cutoff() noexcept
    {
        const double note = 0.0 + (127.0 - 0.0) * (double) x[10];
        double f = (double) (float) (std::pow (2.0, (note - 69.0) / 12.0) * 440.0);
        if ((double) fs / 4.1 < f)
            f = (double) fs / 4.1;
        const double om = (f / (double) fs) * 6.283184;
        a = (float) ((1.0 - std::sin (om)) / std::cos (om));
        b = (float) ((double) (1.0f - a) * 0.5);
    }
    void set (int k, int r) noexcept
    {
        raw[(std::size_t) k] = r;
        const float v = flValue (r, 0, rawMax[(std::size_t) k]);
        x[(std::size_t) k] = v;
        switch (k)
        {
            case 0: delay = truncI (((double) v * 29.95 + 0.05) * (double) fs * 4.096); break;
            case 1: depth = truncI ((double) ((v * 5.0f) * (float) fs) * 4.096 / 64.0); break;
            case 2: stereo = truncI ((double) (v * 4096.0f) * 0.5) - 1; break;
            case 3:
            case 4:
            case 5: inc[k - 3] = (std::uint32_t) truncI ((double) (((v * v) * 83886080.0f) / (float) fs)); break;
            case 6:
            case 7:
            case 8: wave[k - 6] = std::clamp (truncI ((double) (v * 3.0f)), 0, 3); break;
            case 9: lpLine = 0.25 < (double) v; break;
            case 10: cutoff(); break;
            case 11: wetOnly = 0.25 < (double) v; break;
            default: break;
        }
    }
    void reset (std::uint32_t p1, std::uint32_t p2, std::uint32_t p3) noexcept
    {
        st[0] = st[1] = prev[0] = prev[1] = 0.0f;
        line[0].fill (0.0f);
        line[1].fill (0.0f);
        w = 0;
        phase[0] = p1, phase[1] = p2, phase[2] = p3;
    }
    void render (const float* in, float* out, int n) noexcept
    {
        CsrMode mode (0x8040u);
        for (int i = 0; i < n; ++i)
        {
            const float xin[2] { (float) ((double) in[2 * i] * 0.7), (float) ((double) in[2 * i + 1] * 0.7) };
            std::uint32_t off[3][2];
            for (int v = 0; v < 3; ++v)
            {
                const auto& t = tab.t[(std::size_t) wave[v]];
                const std::uint32_t j = (phase[v] >> 12) & 0xfffu;
                const auto lL = (std::uint32_t) t[j], lR = (std::uint32_t) t[(j + (std::uint32_t) stereo) & 0xfffu];
                off[v][0] = (lL * (std::uint32_t) depth) >> 9;
                off[v][1] = (lR * (std::uint32_t) depth) >> 9;
            }
            for (int v = 0; v < 3; ++v)
                phase[v] += inc[v];
            for (int c = 0; c < 2; ++c)
            {
                st[c] = b * (xin[c] + prev[c]) + a * st[c];
                prev[c] = xin[c];
                line[c][(std::size_t) w] = lpLine ? st[c] : xin[c] - st[c];
            }
            const auto base = (std::uint32_t) (w * 4096 - delay);
            float sum[2];
            for (int c = 0; c < 2; ++c)
            {
                const auto& L = line[c];
                float acc = 0.0f;
                for (int v = 0; v < 3; ++v)
                {
                    const std::uint32_t u = base - off[v][c], j = (u >> 12) & lineMask, fr = u & 0xfffu;
                    const float dlt = L[(j + 1) & lineMask] - L[j];
                    const auto val = (float) ((double) L[j] + ((double) dlt * (double) fr) * 0.000244140625);
                    acc = v == 0 ? val : acc + val;
                }
                sum[c] = acc;
            }
            w = (w + 1) & lineMask;
            for (int c = 0; c < 2; ++c)
            {
                const auto o = (float) ((double) sum[c] * 0.6);
                out[2 * i + c] = wetOnly ? o : o + xin[c];
            }
        }
        for (auto& s : st)
            if (std::fabs ((double) s) < 1e-11)
                s = 0.0f;
    }
};

Engine::Engine() : d (std::make_unique<Impl>())
{
    load (defaults);
    d->reset (0, 0, 0);
}
Engine::~Engine() = default;
void Engine::setRate (int sampleRate) noexcept { d->fs = std::max (1, sampleRate); }
void Engine::setParam (int index, int raw) noexcept
{
    if (index >= 0 && index < params)
        d->set (index, raw);
}
void Engine::load (const State& s) noexcept
{
    for (int i = 0; i < params; ++i)
        d->set (i, s[(std::size_t) i]);
}
void Engine::reset (std::uint32_t p1, std::uint32_t p2, std::uint32_t p3) noexcept { d->reset (p1, p2, p3); }
int Engine::param (int index) const noexcept { return d->raw[(std::size_t) index]; }
void Engine::render (const float* in, float* out, int n) noexcept { d->render (in, out, n); }

std::optional<State> parse (const std::vector<std::uint8_t>& state)
{
    if (state.size() < 4 + 4 * params || i32At (state, 0) > 1)
        return std::nullopt;
    State s {};
    for (int i = 0; i < params; ++i)
        s[(std::size_t) i] = i32At (state, 4 + 4 * (std::size_t) i);
    return s;
}
void toParams (const State& s, float* p) noexcept
{
    for (int i = 0; i < params; ++i)
        p[i] = norm (s[(std::size_t) i], 0, rawMax[(std::size_t) i]);
}
State fromParams (const float* p) noexcept
{
    State s {};
    for (int i = 0; i < params; ++i)
        s[(std::size_t) i] = rawFrom (p[i], 0, rawMax[(std::size_t) i]);
    return s;
}
const char* paramName (int param) noexcept
{
    static const char* names[params] { "Delay",      "Depth",      "Stereo",     "LFO 1 freq", "LFO 2 freq",  "LFO 3 freq",
                                       "LFO 1 wave", "LFO 2 wave", "LFO 3 wave", "Cross type", "Cross cutoff", "Wet only" };
    return param >= 0 && param < params ? names[param] : "";
}
std::string paramText (int param, float value01)
{
    char text[32];
    const int p = std::clamp (param, 0, params - 1);
    const int raw = rawFrom (value01, 0, rawMax[(std::size_t) p]);
    const double x = (double) raw / rawMax[(std::size_t) p];
    switch (p)
    {
        case 0: std::snprintf (text, sizeof (text), "%.2f ms", x * 29.95 + 0.05); return text;
        case 1: std::snprintf (text, sizeof (text), "%.2f ms", x * 5.0 * 4.096 / 64.0 * 32766.0 / 512.0); return text;
        case 3:
        case 4:
        case 5: std::snprintf (text, sizeof (text), "%.3f Hz", x * x * 5.0); return text;
        case 6:
        case 7:
        case 8:
        {
            static const char* waves[4] { "parabola", "sharp", "sine", "sharp sine" };
            return waves[std::clamp ((int) (x * 3.0), 0, 3)];
        }
        case 9: return x > 0.25 ? "low" : "high";
        case 10: std::snprintf (text, sizeof (text), "%.0f Hz", 440.0 * std::pow (2.0, (127.0 * x - 69.0) / 12.0)); return text;
        case 11: return x > 0.25 ? "on" : "off";
        default: std::snprintf (text, sizeof (text), "%d%%", (int) std::lround (100.0 * x)); return text;
    }
}

} // namespace fruitychorusfx
} // namespace skelly
