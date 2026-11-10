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
juce::TextButton* findButton(juce::Component& editor,const juce::String& name) {
    for(int i=0;i<editor.getNumChildComponents();++i)
        if(auto* button=dynamic_cast<juce::TextButton*>(editor.getChildComponent(i)))
            if(button->getButtonText()==name)return button;
    return nullptr;
}
}

int main() {
    try {
        juce::ScopedJuceInitialiser_GUI initialiseJuce;
        SkellFlangAudioProcessor processor;
        processor.prepareToPlay(48000.,64);
        std::unique_ptr<juce::AudioProcessorEditor> editor(processor.createEditor());
        verify(editor!=nullptr,"Editor was not created");
        // Headless JUCE components start hidden. They must be visible before
        // paintEntireComponent can draw their child-control hierarchy.
        editor->setVisible(true);
        // attach a real JUCE desktop peer; an unattached component is not
        // "showing" and paintEntireComponent silently skips its child tree.
        editor->addToDesktop(0);
        verify(editor->isShowing(),"Editor must have a visible native peer");
        verify(editor->getWidth()==540,"Compact default width changed unexpectedly");
        verify(processor.getProgramName(0).isNotEmpty(),"VST3 program has no name");
        auto* flanger=findButton(*editor,"FLANGER");
        verify(flanger!=nullptr,"Flanger mode tab missing");
        verify(bool(flanger->onClick),"Mode callback missing");
        flanger->onClick();
        verify(int(processor.parameters.getRawParameterValue("mode")->load())==1,
               "Mode click did not update the processor");
        auto* color=findButton(*editor,"COLOR");
        verify(color!=nullptr && bool(color->onClick),"Color page missing");
        color->onClick();
        verify(int(processor.parameters.state.getProperty("uiPage",-1))==1,
               "Color page did not update");
        bool foundFlangerControl=false;
        for(int i=0;i<editor->getNumChildComponents();++i)
            if(auto* label=dynamic_cast<juce::Label*>(editor->getChildComponent(i)))
                if(label->getText()=="SWEEP RANGE"&&label->isVisible())
                    foundFlangerControl=true;
        verify(foundFlangerControl,"Color page did not expose the mode macro");
        auto* core=findButton(*editor,"CORE");
        verify(core!=nullptr && bool(core->onClick),"Core page missing");
        core->onClick();

        // Shared SkellCore renderer captures the real editor AND child controls.
        // Capture all pages in compact/comfortable/large/studio layouts rather
        // than using an ad-hoc paint() call that misses nested components.
        skellcore::ui::headless::Options options;
        options.project="Skell-Flang actual JUCE AudioProcessorEditor";
        options.minimumColours=12;
        for(const int width : std::array<int,4>{540,720,900,1080}) {
            for(int view=0;view<5;++view) {
                skellcore::ui::headless::Scene scene;
                const juce::String suffix[]={"flanger-core","flanger-color",
                                              "flanger-motion","flanger-morph-full",
                                              "tuned-motion"};
                scene.id="Skell-Flang-"+juce::String(width)+"-"+suffix[view];
                scene.width=width;
                scene.height=juce::roundToInt(double(width)*450./720.);
                scene.mode=view==4?5:1;
                scene.theme=view==0?0:view==1?1:2; // UI page, not a cosmetic theme
                scene.browser=view==3; // Test MORPH at maximum in this view
                options.scenes.push_back(scene);
            }
        }
        options.configure=[&](const auto& scene) {
            auto* modeButton=findButton(*editor,scene.mode==5?"TUNED":"FLANGER");
            verify(modeButton!=nullptr && bool(modeButton->onClick),
                   "Mode tab cannot be selected");
            modeButton->onClick();
            const juce::String names[]{"CORE","COLOR","MOTION"};
            auto* pageButton=findButton(*editor,names[scene.theme]);
            verify(pageButton!=nullptr && bool(pageButton->onClick),
                   "Page tab cannot be selected");
            pageButton->onClick();
            auto* morphSlider=dynamic_cast<juce::Slider*>(editor->findChildWithID("morph"));
            verify(morphSlider!=nullptr,"MORPH control missing");
            morphSlider->setValue(scene.browser?1.0:0.0,juce::sendNotificationSync);
        };
        options.inspect=[&]() {
            juce::StringArray problems;
            const auto bounds=editor->getLocalBounds();
            int visible=0;
            for(int c=0;c<editor->getNumChildComponents();++c) {
                auto* child=editor->getChildComponent(c);
                if(!child || !child->isVisible())continue;
                if(!bounds.contains(child->getBounds()))
                    problems.add("Visible control outside editor: "+child->getName());
                if(child->getWidth()<=5 || child->getHeight()<=5)
                    problems.add("Control too small to interact with: "+child->getName());
                ++visible;
            }
            if(visible<25)problems.add("Too few visible controls");
            return problems;
        };
        options.mustDiffer={
            {"Skell-Flang-540-flanger-core","Skell-Flang-540-flanger-color"},
            {"Skell-Flang-540-flanger-color","Skell-Flang-540-flanger-motion"},
            {"Skell-Flang-540-flanger-motion","Skell-Flang-540-flanger-morph-full"},
            {"Skell-Flang-540-flanger-motion","Skell-Flang-540-tuned-motion"}
        };
        // Windows CI software rendering sanity check before using snapshots.
        juce::Image probe(juce::Image::ARGB,8,8,true);
        {juce::Graphics g(probe);g.fillAll(juce::Colours::red);}
        verify(probe.getPixelAt(4,4).getRed()>200,
               "Windows CI software image rendering unavailable");
        const auto directory=juce::File::getCurrentWorkingDirectory().getChildFile("build/ui-smoke");
        const auto report=skellcore::ui::headless::render(*editor,directory,options);
        for(const auto& error:report.errors)
            std::cerr<<error.toStdString()<<"\\n";
        verify(report.passed(),"Shared SkellCore renderer reported a visual regression");
        verify(report.captured==20,"Expected 20 real-editor JUCE screenshots");
        std::cout<<"SkellCore rendered "<<report.captured
                 <<" real-editor scenes across four sizes and five views\\n";
        juce::AudioBuffer<float> buffer(2,64);
        juce::MidiBuffer midi;
        for(int pass=0;pass<100;++pass) {
            for(int i=0;i<64;++i){
                const float sample=float(.2*std::sin(2*skellflang::pi*440.*(pass*64+i)/48000.));
                buffer.setSample(0,i,sample);buffer.setSample(1,i,-sample);
            }
            processor.processBlock(buffer,midi);
            for(int ch=0;ch<2;++ch)
                for(int i=0;i<64;++i)
                    verify(std::isfinite(buffer.getSample(ch,i)),"Audio output not finite");
        }
        // The final screenshot selects TUNED. An anti-phase stereo sine must
        // still lock: mono summing these channels would erase the fundamental.
        verify(processor.meters.confidence.load(std::memory_order_relaxed)>.78f,
               "Tuned tracker lost anti-phase stereo pitch confidence");
        const float detectedPitch=processor.meters.pitch.load(std::memory_order_relaxed);
        verify(detectedPitch>420.f && detectedPitch<460.f,
               "Tuned tracker missed 440 Hz anti-phase stereo tone");
        processor.releaseResources();
        return 0;
    } catch(const std::exception& exception) {
        std::cerr<<"Skell-Flang UI smoke test failed: "<<exception.what()<<"\n";
        return 1;
    }
}
