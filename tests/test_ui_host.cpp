#include "../Source/PluginProcessor.h"
#include "../Source/PluginEditor.h"
#include <JuceHeader.h>
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
        verify(editor->getWidth()==540,"Compact default width changed unexpectedly");
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
        const std::array<int,4> widths{540,720,900,1080};
        const auto directory=juce::File::getCurrentWorkingDirectory().getChildFile("build/ui-smoke");
        verify(directory.createDirectory().wasOk(),"Cannot create UI test screenshot folder");
        juce::PNGImageFormat png;
        for(const auto width:widths) {
            editor->setSize(width,juce::roundToInt(double(width)*450./720.));
            verify(editor->getWidth()==width,"Editor ignored requested width");
            const auto bounds=editor->getLocalBounds();
            int visible=0;
            for(int c=0;c<editor->getNumChildComponents();++c) {
                auto* child=editor->getChildComponent(c);
                if(child==nullptr||!child->isVisible())continue;
                verify(bounds.contains(child->getBounds()),"Visible control outside the editor");
                verify(child->getWidth()>5&&child->getHeight()>5,
                       "Visible control too small to interact with");
                ++visible;
            }
            verify(visible>=25,"Too few visible controls");
            juce::Image rendered(juce::Image::ARGB,width,editor->getHeight(),true);
            juce::Graphics graphics(rendered);
            editor->paintEntireComponent(graphics,true);
            const auto top=rendered.getPixelAt(width/2,2);
            const auto center=rendered.getPixelAt(width/2,editor->getHeight()/2);
            verify(top!=center,"Rendered editor appears uniformly blank");
            const auto file=directory.getChildFile("Skell-Flang-"+juce::String(width)+".png");
            auto stream=file.createOutputStream();
            verify(stream!=nullptr && png.writeImageToStream(rendered,*stream),
                   "Failed to write rendered UI snapshot");
            std::cout<<"Rendered and checked "<<width<<" x "<<editor->getHeight()
                     <<" with "<<visible<<" visible controls\n";
        }
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
        processor.releaseResources();
        return 0;
    } catch(const std::exception& exception) {
        std::cerr<<"Skell-Flang UI smoke test failed: "<<exception.what()<<"\n";
        return 1;
    }
}
