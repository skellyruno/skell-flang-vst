#include "../Source/PluginProcessor.h"
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <new>

#if defined(_MSC_VER)
 #include <malloc.h>
#endif

namespace {
class TestPlayHead final : public juce::AudioPlayHead {
public:
    juce::Optional<juce::AudioPlayHead::PositionInfo> getPosition() const override {
        juce::AudioPlayHead::PositionInfo position;
        position.setBpm(120.0);
        return juce::Optional<juce::AudioPlayHead::PositionInfo>(position);
    }
};

thread_local bool trackingAllocations = false;
thread_local std::size_t trackedNewCalls = 0;

void recordNewCall() noexcept {
    if (trackingAllocations)
        ++trackedNewCalls;
}

void* allocate(std::size_t size) {
    if (auto* memory = std::malloc(size == 0 ? 1 : size)) {
        recordNewCall();
        return memory;
    }
    throw std::bad_alloc();
}

void* allocateAligned(std::size_t size, std::size_t alignment) {
    void* memory = nullptr;
#if defined(_MSC_VER)
    memory = _aligned_malloc(size == 0 ? 1 : size, alignment);
#else
    if (::posix_memalign(&memory, alignment, size == 0 ? 1 : size) != 0)
        memory = nullptr;
#endif
    if (memory == nullptr)
        throw std::bad_alloc();
    recordNewCall();
    return memory;
}

void freeAligned(void* memory) noexcept {
#if defined(_MSC_VER)
    _aligned_free(memory);
#else
    std::free(memory);
#endif
}

struct AllocationProbe {
    AllocationProbe() noexcept
        : previousTracking(trackingAllocations), before(trackedNewCalls) {
        trackingAllocations = true;
    }

    AllocationProbe(const AllocationProbe&) = delete;
    AllocationProbe& operator=(const AllocationProbe&) = delete;

    std::size_t finish() noexcept {
        trackingAllocations = previousTracking;
        active = false;
        return trackedNewCalls - before;
    }

    ~AllocationProbe() {
        if (active)
            trackingAllocations = previousTracking;
    }

private:
    bool previousTracking = false;
    std::size_t before = 0;
    bool active = true;
};

bool setParameter(SkellFlangAudioProcessor& processor,
                  const char* id,
                  float value) {
    auto* parameter = processor.parameters.getParameter(id);
    if (parameter == nullptr) {
        std::fprintf(stderr, "Missing required parameter: %s\n", id);
        return false;
    }
    parameter->setValueNotifyingHost(parameter->convertTo0to1(value));
    return true;
}

constexpr std::array<int, 7> blockSizes{{0, 1, 17, 64, 128, 512, 1024}};
constexpr double pi = 3.1415926535897932384626433832795;

bool processOne(SkellFlangAudioProcessor& processor,
                juce::AudioBuffer<float>& buffer,
                juce::MidiBuffer& midi,
                double sampleRate,
                std::uint64_t& frame,
                std::uint64_t& blocks,
                const char* scenario) {
    const int samples = buffer.getNumSamples();
    for (int channel = 0; channel < buffer.getNumChannels(); ++channel) {
        auto* data = buffer.getWritePointer(channel);
        for (int i = 0; i < samples; ++i) {
            const double phase = 2.0 * pi * 220.0 *
                static_cast<double>(frame + static_cast<std::uint64_t>(i)) /
                sampleRate;
            const float sample = 0.20f * static_cast<float>(std::sin(phase));
            data[i] = channel == 1 ? -sample : sample;
        }
    }

    std::size_t allocations = 0;
    {
        AllocationProbe probe;
        processor.processBlock(buffer, midi);
        allocations = probe.finish();
    }

    if (allocations != 0) {
        std::fprintf(stderr,
                     "FAIL: %s allocated %zu C++ object(s) in processBlock "
                     "(channels=%d, samples=%d, rate=%.0f)\n",
                     scenario, allocations, buffer.getNumChannels(),
                     samples, sampleRate);
        return false;
    }

    for (int channel = 0; channel < buffer.getNumChannels(); ++channel) {
        const auto* data = buffer.getReadPointer(channel);
        for (int i = 0; i < samples; ++i) {
            if (!std::isfinite(data[i])) {
                std::fprintf(stderr,
                             "FAIL: %s produced non-finite output "
                             "(channels=%d, samples=%d, rate=%.0f, index=%d)\n",
                             scenario, buffer.getNumChannels(), samples,
                             sampleRate, i);
                return false;
            }
        }
    }

    frame += static_cast<std::uint64_t>(samples);
    ++blocks;
    return true;
}

bool processBuffers(SkellFlangAudioProcessor& processor,
                    juce::MidiBuffer& midi,
                    std::array<juce::AudioBuffer<float>, blockSizes.size()>& mono,
                    std::array<juce::AudioBuffer<float>, blockSizes.size()>& stereo,
                    double sampleRate,
                    std::uint64_t& frame,
                    std::uint64_t& blocks,
                    const char* scenario,
                    int passes) {
    for (int pass = 0; pass < passes; ++pass) {
        for (std::size_t i = 0; i < blockSizes.size(); ++i) {
            if (!processOne(processor, mono[i], midi, sampleRate,
                            frame, blocks, scenario))
                return false;
            if (!processOne(processor, stereo[i], midi, sampleRate,
                            frame, blocks, scenario))
                return false;
        }
    }
    return true;
}
} // namespace

void* operator new(std::size_t size) {
    return allocate(size);
}
void* operator new[](std::size_t size) {
    return allocate(size);
}
void operator delete(void* memory) noexcept {
    std::free(memory);
}
void operator delete[](void* memory) noexcept {
    std::free(memory);
}
void operator delete(void* memory, std::size_t) noexcept {
    std::free(memory);
}
void operator delete[](void* memory, std::size_t) noexcept {
    std::free(memory);
}
void* operator new(std::size_t size, std::align_val_t alignment) {
    return allocateAligned(size, static_cast<std::size_t>(alignment));
}
void* operator new[](std::size_t size, std::align_val_t alignment) {
    return allocateAligned(size, static_cast<std::size_t>(alignment));
}
void operator delete(void* memory, std::align_val_t) noexcept {
    freeAligned(memory);
}
void operator delete[](void* memory, std::align_val_t) noexcept {
    freeAligned(memory);
}
void operator delete(void* memory, std::size_t, std::align_val_t) noexcept {
    freeAligned(memory);
}
void operator delete[](void* memory, std::size_t, std::align_val_t) noexcept {
    freeAligned(memory);
}

int main() {
    try {
        // Exercise the host-playhead branch used by DAWs while the allocation
        // probe is active, rather than testing only the no-host fallback.
        TestPlayHead playHead;
        SkellFlangAudioProcessor processor;
        processor.setPlayHead(&playHead);
        juce::MidiBuffer midi;

        std::array<juce::AudioBuffer<float>, blockSizes.size()> mono;
        std::array<juce::AudioBuffer<float>, blockSizes.size()> stereo;
        for (std::size_t i = 0; i < blockSizes.size(); ++i) {
            mono[i].setSize(1, blockSizes[i]);
            stereo[i].setSize(2, blockSizes[i]);
        }

        std::uint64_t frame = 0;
        std::uint64_t blocks = 0;
        constexpr std::array<double, 4> rates{{44100.0, 48000.0, 96000.0, 192000.0}};

        for (const double sampleRate : rates) {
            processor.prepareToPlay(sampleRate, 1024);

            if (!setParameter(processor, "power", 1.0f) ||
                !setParameter(processor, "layerMode", 0.0f) ||
                !setParameter(processor, "order", 8.0f) ||
                !setParameter(processor, "depth", 0.88f) ||
                !setParameter(processor, "speed", 0.62f) ||
                !setParameter(processor, "delay", 0.74f) ||
                !setParameter(processor, "spread", 0.86f) ||
                !setParameter(processor, "feedback", 0.85f) ||
                !setParameter(processor, "morph", 0.45f) ||
                !setParameter(processor, "lfoShape", 4.0f))
                return 2;

            for (int mode = 0; mode < 8; ++mode) {
                if (!setParameter(processor, "mode", static_cast<float>(mode)) ||
                    !setParameter(processor, "morphTarget",
                                  static_cast<float>((mode + 1) % 8)) ||
                    !setParameter(processor, "feedback",
                                  (mode & 1) ? -0.85f : 0.85f) ||
                    !processBuffers(processor, midi, mono, stereo, sampleRate,
                                    frame, blocks, "single-mode+morph", 2))
                    return 3;
            }

            // Legacy Tuned and morph-to-Tuned both enter the pitch tracker path.
            if (!setParameter(processor, "layerMode", 0.0f) ||
                !setParameter(processor, "mode", 5.0f) ||
                !setParameter(processor, "morphTarget", 5.0f) ||
                !setParameter(processor, "morph", 0.0f) ||
                !processBuffers(processor, midi, mono, stereo, sampleRate,
                                frame, blocks, "legacy-tuned", 2) ||
                !setParameter(processor, "mode", 0.0f) ||
                !setParameter(processor, "morphTarget", 5.0f) ||
                !setParameter(processor, "morph", 0.65f) ||
                !processBuffers(processor, midi, mono, stereo, sampleRate,
                                frame, blocks, "morph-to-tuned", 2))
                return 4;

            // Exercise all five independent layer engines, then the safe
            // all-disabled fallback used during rapid host automation.
            if (!setParameter(processor, "layerMode", 1.0f) ||
                !setParameter(processor, "layerChorus", 1.0f) ||
                !setParameter(processor, "layerFlanger", 1.0f) ||
                !setParameter(processor, "layerFlangus", 1.0f) ||
                !setParameter(processor, "layerOrbit", 1.0f) ||
                !setParameter(processor, "layerDimension", 1.0f) ||
                !setParameter(processor, "morph", 0.75f) ||
                !processBuffers(processor, midi, mono, stereo, sampleRate,
                                frame, blocks, "layer-stack-all", 2) ||
                !setParameter(processor, "layerChorus", 0.0f) ||
                !setParameter(processor, "layerFlanger", 0.0f) ||
                !setParameter(processor, "layerFlangus", 0.0f) ||
                !setParameter(processor, "layerOrbit", 0.0f) ||
                !setParameter(processor, "layerDimension", 0.0f) ||
                !processBuffers(processor, midi, mono, stereo, sampleRate,
                                frame, blocks, "layer-stack-fallback", 1))
                return 5;

            if (!setParameter(processor, "layerMode", 0.0f) ||
                !setParameter(processor, "power", 0.0f) ||
                !setParameter(processor, "mode", 0.0f) ||
                !setParameter(processor, "morph", 0.0f) ||
                !processBuffers(processor, midi, mono, stereo, sampleRate,
                                frame, blocks, "bypass", 1))
                return 6;

            processor.releaseResources();
        }

        std::printf("PASS: %llu direct processBlock calls; zero C++ new/new[]/"
                    "aligned-new allocations; host BPM playhead path; finite mono/stereo output; "
                    "44.1/48/96/192 kHz; block sizes 0/1/17/64/128/512/1024.\n",
                    static_cast<unsigned long long>(blocks));
        return 0;
    } catch (const std::exception& error) {
        trackingAllocations = false;
        std::fprintf(stderr, "FAIL: processBlock allocation gate threw: %s\n",
                     error.what());
        return 7;
    } catch (...) {
        trackingAllocations = false;
        std::fputs("FAIL: processBlock allocation gate threw an unknown exception\n",
                   stderr);
        return 8;
    }
}
