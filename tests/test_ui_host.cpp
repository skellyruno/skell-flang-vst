#include "../Source/PluginProcessor.h"
#include "../Source/PluginEditor.h"
#include <JuceHeader.h>
#include <SkellCore/HeadlessUIRenderer.h>
#include <array>
#include <cmath>
#include <iostream>
#include <memory>
#include <stdexcept>

namespace {
void verify(bool ok,const char* message) {
    if(!ok)throw std::runtime_error(message);
}
juce::TextButton* findButton(juce::Component& editor,const juce::String& text) {
    for(int i=0;i<editor.getNumChildComponents();++i)
        if(auto* b=dynamic_cast<juce::TextButton*>(editor.getChildComponent(i)))
            if(b->getButtonText()==text)return b;
    return nullptr;
}
juce::TextButton* byID(juce::Component& editor,const char* id) {
    return dynamic_cast<juce::TextButton*>(editor.findChildWithID(id));
}
void click(juce::TextButton* b,const char* error){
    verify(b!=nullptr && bool(b->onClick),error);
    b->onClick();
}
}

int main(){
    try {
        juce::ScopedJuceInitialiser_GUI gui;
        SkellFlangAudioProcessor processor;
        processor.prepareToPlay(48000.,64);
        std::unique_ptr<juce::AudioProcessorEditor> editor(processor.createEditor());
        verify(editor!=nullptr,"Actual editor was not created");
        editor->setVisible(true);
        editor->addToDesktop(0);
        verify(editor->isShowing(),"Editor needs a native peer for real screenshots");
        verify(editor->getWidth()==540,"Compact size changed unexpectedly");
        verify(processor.getProgramName(0).isNotEmpty(),"Missing host program name");

        const std::array<int,5> choices{0,1,6,4,7};
        const std::array<const char*,5> names{"CHORUS","FLANGER","FLANGUS","ORBIT","DIMENSION"};
        for(size_t i=0;i<choices.size();++i) {
            auto* tab=findButton(*editor,names[i]);
            click(tab,"Missing effect tab or callback");
            verify(int(processor.parameters.getRawParameterValue("mode")->load())==choices[i],
                   "Displayed mode ID does not match the host parameter choice");
        }
        for(const char* old:{"VOWEL","REVERSE","TUNED"})
            verify(findButton(*editor,old)==nullptr,"Removed mode visible in selector");
        auto* skull=byID(*editor,"skull");
        auto* cog=byID(*editor,"settings");
        auto* preset=byID(*editor,"preset");
        verify(skull&&cog&&preset,"Missing skull, gear or preset control");
        verify(bool(skull->onClick)&&bool(cog->onClick)&&bool(preset->onClick),
               "An interactive header control has no callback");
        verify(byID(*editor,"mod")==nullptr && findButton(*editor,"MOD")==nullptr &&
               findButton(*editor,"BACK")==nullptr,
               "Obsolete MOD drawer button is still present");

        auto* morph=dynamic_cast<juce::Slider*>(editor->findChildWithID("morph"));
        auto* dest=dynamic_cast<juce::ComboBox*>(editor->findChildWithID("morph-target"));
        auto* wave=dynamic_cast<juce::ComboBox*>(editor->findChildWithID("lfo-wave"));
        auto* sync=byID(*editor,"lfo-sync");
        verify(morph&&dest&&wave&&sync,"Missing permanently integrated morph/LFO controls");
        verify(morph->isVisible()&&dest->isVisible()&&wave->isVisible()&&sync->isVisible(),
               "Morph and LFO controls must always be visible");
        click(skull,"Skull theme cycle is not clickable");
        verify(morph->isVisible(),"Skull theme change hid morph controls");

        // A factory preset must update the engine AND the character controls.
        processor.applyFactoryPreset(7);
        verify(int(processor.parameters.getRawParameterValue("mode")->load())==6,
               "Flangus preset did not select Flangus");
        verify(processor.parameters.getRawParameterValue("extra0")->load()>.6f,
               "Flangus preset did not load its own sound macros");
        verify(processor.parameters.state.getProperty("presetName").toString()=="Flangus Glow",
               "Preset title was not saved");
        verify(processor.getLatencySamples()==0,"Unexpected host delay compensation");

        // Shared SkellCore captures the ACTUAL editor child tree at every
        // supported size, across all five modes, with both dry and morphed settings.
        skellcore::ui::headless::Options options;
        options.project="Skell-Flang v0.8 real JUCE AudioProcessorEditor";
        options.minimumColours=12;
        for(const int width:std::array<int,4>{540,720,900,1080})
            for(size_t mode=0;mode<choices.size();++mode)
                for(int morphed=0;morphed<2;++morphed) {
                    skellcore::ui::headless::Scene scene;
                    scene.id="Skell-Flang-"+juce::String(width)+"-"+
                              juce::String(names[mode]).toLowerCase()+
                              (morphed?"-morphed":"-unmorphed");
                    scene.width=width;
                    scene.height=juce::roundToInt(double(width)*450./720.);
                    scene.mode=choices[mode];
                    scene.browser=morphed!=0;
                    options.scenes.push_back(scene);
                }
        options.configure=[&](const auto& scene) {
            auto* tab=findButton(*editor,juce::String(
                scene.mode==0?"CHORUS":scene.mode==1?"FLANGER":
                scene.mode==6?"FLANGUS":scene.mode==4?"ORBIT":"DIMENSION"));
            click(tab,"Mode screenshot selector could not be clicked");
            verify(int(processor.parameters.getRawParameterValue("mode")->load())==scene.mode,
                   "Wrong mode in screenshot");
            morph->setValue(scene.browser?.76:0.0,juce::sendNotificationSync);
            verify(morph->isVisible()&&dest->isVisible()&&wave->isVisible()&&sync->isVisible(),
                   "Controls hidden after mode or morph change");
        };
        options.inspect=[&]{
            juce::StringArray errors;
            const auto bounds=editor->getLocalBounds();
            int count=0;
            for(int i=0;i<editor->getNumChildComponents();++i) {
                auto* c=editor->getChildComponent(i);
                if(!c||!c->isVisible())continue;
                if(!bounds.contains(c->getBounds()))
                    errors.add("Control out of bounds: "+c->getComponentID());
                if(c->getWidth()<=5||c->getHeight()<=5)
                    errors.add("Control too small: "+c->getComponentID());
                ++count;
            }
            if(count<25)errors.add("Too few visible controls");
            // All interactive controls must have independent, non-overlapping
            // mouse hit areas, even at the compact 540 px editor width.
            for(int i=0;i<editor->getNumChildComponents();++i) {
                auto* a=editor->getChildComponent(i);
                if(!a||!a->isVisible()||
                    !(dynamic_cast<juce::Slider*>(a)||
                      dynamic_cast<juce::ComboBox*>(a)||
                      dynamic_cast<juce::TextButton*>(a)))continue;
                for(int j=i+1;j<editor->getNumChildComponents();++j) {
                    auto* b=editor->getChildComponent(j);
                    if(!b||!b->isVisible()||
                        !(dynamic_cast<juce::Slider*>(b)||
                          dynamic_cast<juce::ComboBox*>(b)||
                          dynamic_cast<juce::TextButton*>(b)))continue;
                    if(a->getBounds().intersects(b->getBounds()))
                        errors.add("Overlapping controls: "+a->getComponentID()+" / "+b->getComponentID());
                }
            }
            return errors;
        };
        options.mustDiffer={
            {"Skell-Flang-540-chorus-unmorphed","Skell-Flang-540-chorus-morphed"},
            {"Skell-Flang-540-flanger-unmorphed","Skell-Flang-540-flangus-unmorphed"},
            {"Skell-Flang-540-orbit-unmorphed","Skell-Flang-540-dimension-unmorphed"}
        };
        juce::Image probe(juce::Image::ARGB,8,8,true);
        {juce::Graphics g(probe);g.fillAll(juce::Colours::red);}
        verify(probe.getPixelAt(4,4).getRed()>200,
               "CI runner software rendering unavailable");

        const auto output=juce::File::getCurrentWorkingDirectory().getChildFile("build/ui-smoke");
        const auto report=skellcore::ui::headless::render(*editor,output,options);
        for(const auto& error:report.errors)
            std::cerr<<"Visual regression: "<<error.toStdString()<<"\n";
        verify(report.passed(),"Shared SkellCore visual regression failed");
        verify(report.captured==40,"Expected 40 screenshots at four widths");
        std::cout<<"SkellCore captured "<<report.captured<<" real-editor views\n";

        // Keep testing legacy Tuned playback for old FL Studio saves, although
        // that mode no longer appears as a tab in the current interface.
        if(auto* v=processor.parameters.getParameter("mode"))
            v->setValueNotifyingHost(v->convertTo0to1(5.f));
        juce::AudioBuffer<float> audio(2,64);
        juce::MidiBuffer midi;
        for(int pass=0;pass<100;++pass) {
            for(int i=0;i<64;++i){
                const float input=float(.2*std::sin(2*skellflang::pi*440.*(pass*64+i)/48000.));
                audio.setSample(0,i,input);audio.setSample(1,i,-input);
            }
            processor.processBlock(audio,midi);
            for(int ch=0;ch<2;++ch)
                for(int i=0;i<64;++i)
                    verify(std::isfinite(audio.getSample(ch,i)),"Non-finite audio output");
        }
        verify(processor.meters.confidence.load()>.78f,
               "Legacy tuned tracker lost anti-phase stereo note");
        processor.releaseResources();
        return 0;
    }catch(const std::exception& e){
        std::cerr<<"Skell-Flang v0.8 UI test failed: "<<e.what()<<"\n";
        return 1;
    }
}
