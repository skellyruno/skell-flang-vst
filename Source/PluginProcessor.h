#pragma once
#include <JuceHeader.h>
#include "FlangDSP.h"
#include <SkellCore/WorldLink.h>
#include <array>
#include <atomic>

class SkellFlangAudioProcessor final : public juce::AudioProcessor {
public:
    SkellFlangAudioProcessor();
    ~SkellFlangAudioProcessor() override = default;
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported(const BusesLayout&) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "Skell-Flang"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 1.5; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    // VST3 hosts enumerate program slot 0 even though factory presets live
    // in the plugin's own browser. A blank name fails Steinberg's validator.
    const juce::String getProgramName(int index) override {
        return index==0 ? juce::String("Current") : juce::String();
    }
    void changeProgramName(int,const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*,int) override;
    void applyFactoryPreset(int);
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    // Opt-in background-only presence. The audio callback never calls WorldLink.
    skellcore::world::WorldLink worldLink{"skell.flang","Skell-Flang"};
    juce::AudioProcessorValueTreeState parameters;
    struct MeterData {
        std::atomic<float> peaks[2];
        std::atomic<float> scope[64];
        std::atomic<unsigned> scopeNext{0};
        std::atomic<float> bpm{120.f},cpu{0.f},pitch{0.f},confidence{0.f};
        MeterData() {
            for(auto& x:peaks)x.store(0);
            for(auto& x:scope)x.store(0);
        }
    } meters;
private:
    float raw(const char*) const noexcept;
    skellflang::Controls settings(int mode) const noexcept;
    skellflang::EngineBank engines;
    skellflang::PitchTracker pitchTracker;
    skellflang::EqualPowerBlend crossfade;
    std::array<skellflang::ControlSmoother,8> controlSmoothers;
    juce::SmoothedValue<float,juce::ValueSmoothingTypes::Linear> dryMix,wetMix,outputMix,powerMix,morphMix,crossMix;
    int active=0,previous=0,remaining=0,transition=1;
    float holdL=0,holdR=0,cpuDisplay=0.f;
    int scopeStride=32,scopeCounter=0;
    unsigned scopeIndex=0;
    double rate=48000.0;
    bool pitchFromSide=false;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SkellFlangAudioProcessor)
};
