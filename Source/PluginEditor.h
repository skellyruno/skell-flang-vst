#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include <array>
#include <memory>

class FlangLookAndFeel final : public juce::LookAndFeel_V4 {
public:
    juce::Colour accent=juce::Colour::fromRGB(57,255,58);
    void drawLinearSlider(juce::Graphics&,int,int,int,int,float,float,float,
                          const juce::Slider::SliderStyle,juce::Slider&) override;
    void drawRotarySlider(juce::Graphics&,int,int,int,int,float,float,float,juce::Slider&) override;
    void drawButtonBackground(juce::Graphics&,juce::Button&,const juce::Colour&,
                              bool,bool) override;
};
class SkellFlangAudioProcessorEditor final : public juce::AudioProcessorEditor,
                                              private juce::Timer {
public:
    explicit SkellFlangAudioProcessorEditor(SkellFlangAudioProcessor&);
    ~SkellFlangAudioProcessorEditor() override;
    void paint(juce::Graphics&) override;
    void resized() override;
private:
    void timerCallback() override;
    void updateMode();
    void updatePage();
    SkellFlangAudioProcessor& processor;
    FlangLookAndFeel skin;
    std::array<juce::Slider,7> faders;
    std::array<juce::Label,7> faderLabels;
    std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>,7> faderLinks;
    juce::Slider order,morph;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> orderLink,morphLink;
    std::array<juce::Slider,7> knobs;
    std::array<juce::Label,7> knobLabels;
    std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>,7> knobLinks;
    std::array<juce::TextButton,6> tabs;
    juce::ComboBox target,shape,root,scale,tuneSource;
    std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>,5> comboLinks;
    juce::ToggleButton power{"POWER"},sync{"SYNC"};
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> powerLink,syncLink;
    juce::TextButton preset{"INIT"}, settings{"SIZE"};
    std::array<juce::TextButton,3> pages;
    int page=0;
    int shownMode=-1;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SkellFlangAudioProcessorEditor)
};
