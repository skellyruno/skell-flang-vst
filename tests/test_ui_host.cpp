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
        const std::array<int,4> widths{540,720,900,1080};
        const auto directory=juce::File::getCurrentWorkingDirectory().getChildFile("build/ui-smoke");
        verify(directory.createDirectory().wasOk(),"Cannot create UI test screenshot folder");
        juce::PNGImageFormat png;
        // Verify the unattended runner can actually draw into an ARGB image.
        // Otherwise a black/transparent snapshot is a test-environment error.
        juce::Image probe(juce::Image::ARGB,8,8,true);
        {
            juce::Graphics g(probe);
            g.fillAll(juce::Colours::red);
        }
        verify(probe.getPixelAt(4,4).getRed()>200,
               "The Windows CI runner cannot render to JUCE software images");
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
            {
                // The software/native graphics context must be closed before
                // getPixelAt or PNG encoding: Windows may defer writes until
                // the Graphics destructor flushes the backing image.
                juce::Graphics graphics(rendered);
                editor->paint(graphics);
                for(int c=0;c<editor->getNumChildComponents();++c) {
                    auto* child=editor->getChildComponent(c);
                    if(child==nullptr||!child->isVisible())continue;
                    graphics.saveState();
                    graphics.addTransform(juce::AffineTransform::translation(
                        float(child->getX()),float(child->getY())));
                    graphics.reduceClipRegion(child->getLocalBounds());
                    child->paint(graphics);
                    graphics.restoreState();
                }
            }
            const auto top=rendered.getPixelAt(width/2,2);
            const auto center=rendered.getPixelAt(width/2,editor->getHeight()/2);
            const auto file=directory.getChildFile("Skell-Flang-"+juce::String(width)+".png");
            auto stream=file.createOutputStream();
            verify(stream!=nullptr && png.writeImageToStream(rendered,*stream),
                   "Failed to write rendered UI snapshot");
            if(top==center) {
                std::cout<<"At "<<width<<", top rgba "<<top.toDisplayString(true)
                         <<", centre rgba "<<center.toDisplayString(true)<<"\n";
                verify(false,"Rendered editor appears uniformly blank");
            }
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
