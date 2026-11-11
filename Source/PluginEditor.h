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
    juce::Font getTextButtonFont(juce::TextButton&,int buttonHeight) override;
    juce::Font getComboBoxFont(juce::ComboBox&) override;
    juce::Font getPopupMenuFont() override;
    void drawPopupMenuBackground(juce::Graphics&,int,int) override;
    void drawPopupMenuSectionHeader(juce::Graphics&,const juce::Rectangle<int>&,
                                    const juce::String&) override;
    void drawPopupMenuItem(juce::Graphics&,const juce::Rectangle<int>&,
        bool,bool,bool,bool,bool,const juce::String&,const juce::String&,
        const juce::Drawable*,const juce::Colour*) override;
    void getIdealPopupMenuItemSize(const juce::String&,bool,int,int&,int&) override;
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
    std::array<juce::TextButton,5> tabs;
    juce::ComboBox target,shape;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> shapeLink;
    juce::TextButton power{"ACTIVE"},sync{"SYNC"};
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> powerLink,syncLink;
    juce::TextButton preset{"INIT"}, settings{""}, skull{""};
    float headerLevel=0.f;
    int theme=0;
    int shownMode=-1;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SkellFlangAudioProcessorEditor)
};
