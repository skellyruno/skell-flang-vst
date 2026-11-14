#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include <array>
#include <memory>

class FlangLookAndFeel final : public juce::LookAndFeel_V4 {
public:
    // The skull changes the entire surface palette. The selected effect
    // always keeps its own accent, independent of which skin is active.
    void setTheme(int index);
    int themeIndex=0;
    juce::Colour accent=juce::Colour::fromRGB(57,255,58);
    juce::Colour background=juce::Colour::fromRGB(7,12,14);
    juce::Colour surface=juce::Colour::fromRGB(17,26,29);
    juce::Colour foreground=juce::Colour::fromRGB(226,239,230);
    juce::Colour secondary=juce::Colour::fromRGB(143,169,164);
    juce::Colour outline=juce::Colour::fromRGB(33,54,51);
    juce::Colour recess=juce::Colour::fromRGB(8,17,19);
    void drawLinearSlider(juce::Graphics&,int,int,int,int,float,float,float,
                          const juce::Slider::SliderStyle,juce::Slider&) override;
    void drawRotarySlider(juce::Graphics&,int,int,int,int,float,float,float,juce::Slider&) override;
    void drawButtonBackground(juce::Graphics&,juce::Button&,const juce::Colour&,
                              bool,bool) override;
    void drawButtonText(juce::Graphics&,juce::TextButton&,bool,bool) override;
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
    void updateMacroSelection();

    SkellFlangAudioProcessor& processor;
    FlangLookAndFeel skin;
    std::array<juce::Slider,7> faders;
    std::array<juce::Label,7> faderLabels;
    std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>,7> faderLinks;
    juce::Slider order;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> orderLink;
    std::array<juce::Slider,7> knobs;
    std::array<juce::Label,7> knobLabels;
    // Keep four real APVTS attachments, but only show one macro dial at a time.
    std::array<juce::TextButton,4> macroTabs;
    int selectedMacro=0;
    std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>,7> knobLinks;
    std::array<juce::TextButton,5> tabs;
    std::array<bool,5> shownLayers{{}};
    juce::ComboBox shape;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> shapeLink;
    juce::TextButton power{"ACTIVE"},sync{"SYNC"};
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> powerLink,syncLink;
    juce::TextButton preset{"INIT"}, settings{""}, skull{""};
    float headerLevel=0.f;
    int theme=0;
    int shownMode=-1;
    int shownTheme=-1;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SkellFlangAudioProcessorEditor)
};
