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

int main(int argc,char** argv){
    bool full=false;
    juce::String capturePath="build/ui-smoke";
    for(int i=1;i<argc;++i){
        const juce::String arg(argv[i]);
        if(arg=="--full")full=true;
        else if(arg=="--output" && i+1<argc)capturePath=argv[++i];
        else {std::cerr<<"Unknown Quick4 argument: "<<arg.toStdString()<<"\n";return 2;}
    }
    try {
        juce::ScopedJuceInitialiser_GUI gui;
        SkellFlangAudioProcessor processor;
        processor.prepareToPlay(48000.,64);
        std::unique_ptr<juce::AudioProcessorEditor> editor(processor.createEditor());
        verify(editor!=nullptr,"Actual editor was not created");
        editor->setVisible(true);
        // Quick4 captures the *actual* JUCE component tree offscreen, like
        // Mini/Master: no X11 desktop peer (some Linux/Xvfb stacks reject the
        // X_ChangeProperty atom). The full legacy host test still uses a peer.
        if(full) {
            editor->addToDesktop(0);
            verify(editor->isShowing(),"Editor needs a native peer for real screenshots");
        }
        verify(editor->getWidth()==540 && editor->getHeight()==375,
               "Compact 540 x 375 editor proportions are incorrect");
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
        // Compact controls are a deliberate UI contract. A wide gear or
        // oversized effect row would obscure the hardware-style rack header.
        verify(cog->getWidth()<=34 && cog->getHeight()<=30,
               "Settings button regressed to an oversized header block");
        verify(preset->getWidth()>170 && preset->getRight()<cog->getX(),
               "Sound library selector must be legible and clear of settings");
        for(const auto* name:names) {
            auto* tab=findButton(*editor,name);
            verify(tab && tab->getHeight()<=24 && tab->getWidth()<=85,
                   "Effect mode selector is no longer the compact design");
        }
        verify(byID(*editor,"mod")==nullptr && findButton(*editor,"MOD")==nullptr &&
               findButton(*editor,"BACK")==nullptr,
               "Obsolete MOD drawer button is still present");

        auto* morph=dynamic_cast<juce::Slider*>(editor->findChildWithID("morph"));
        auto* dest=dynamic_cast<juce::ComboBox*>(editor->findChildWithID("morph-target"));
        auto* wave=dynamic_cast<juce::ComboBox*>(editor->findChildWithID("lfo-wave"));
        auto* sync=byID(*editor,"lfo-sync");
        verify(morph&&dest&&wave&&sync,"Missing permanently integrated morph/LFO controls");
        verify(dest->getSelectedId()==2,
               "Initial morph target must be populated without waiting for the UI timer");
        verify(morph->isVisible()&&dest->isVisible()&&wave->isVisible()&&sync->isVisible(),
               "Morph and LFO controls must always be visible");
        auto* look=dynamic_cast<FlangLookAndFeel*>(&editor->getLookAndFeel());
        verify(look!=nullptr,"Skell-Flang is not using the custom look-and-feel");
        const auto initialAccent=look->accent;
        const auto initialBackground=look->background;
        click(skull,"Skull theme cycle is not clickable");
        verify(look->background!=initialBackground,
               "Skull click did not actually change the interface background");
        verify(look->accent==initialAccent,
               "Skull theme click incorrectly changed the effect identity accent");
        verify(morph->isVisible(),"Skull theme change hid morph controls");
        // The four advanced character parameters must be selectable without
        // drawing seven simultaneous rotary knobs. Every original APVTS dial
        // remains instantiated and available to the DAW automation system.
        std::array<juce::TextButton*,4> macroSelectors{};
        std::array<juce::Slider*,4> macroDials{};
        for(int i=0;i<4;++i) {
            macroSelectors[size_t(i)]=byID(*editor,
                ("macro-"+juce::String(i)).toRawUTF8());
            macroDials[size_t(i)]=dynamic_cast<juce::Slider*>(
                editor->findChildWithID("tone-"+juce::String(i+3)));
            verify(macroSelectors[size_t(i)] && macroDials[size_t(i)],
                   "A character macro or its original parameter dial is missing");
        }
        for(int i=0;i<4;++i) {
            click(macroSelectors[size_t(i)],"Character selector does not respond");
            int visible=0;
            for(int n=0;n<4;++n)
                if(macroDials[size_t(n)]->isVisible())++visible;
            verify(visible==1&&macroDials[size_t(i)]->isVisible(),
                   "Character selection must expose exactly one correct dial");
            verify(macroSelectors[size_t(i)]->getToggleState(),
                   "Character selector did not illuminate its active pad");
        }
        click(macroSelectors[1],"Cannot select second character macro");
        const auto selectedBefore=macroDials[1]->isVisible();
        click(skull,"Cannot change theme with a contextual macro selected");
        verify(selectedBefore&&macroDials[1]->isVisible(),
               "Theme switching changed the selected macro");
        click(macroSelectors[0],"Cannot restore default macro selector");

        // A factory preset must update the engine AND the character controls.
        processor.applyFactoryPreset(7);
        verify(int(processor.parameters.getRawParameterValue("mode")->load())==6,
               "Flangus preset did not select Flangus");
        verify(processor.parameters.getRawParameterValue("extra0")->load()>.6f,
               "Flangus preset did not load its own sound macros");
        verify(processor.parameters.state.getProperty("presetName").toString()=="Flangus Glow",
               "Preset title was not saved");
        verify(processor.getLatencySamples()==0,"Unexpected host delay compensation");
        // WorldLink is opt-in and owned by the processor, not the editor.
        // Exercise a no-hub join/leave while the native editor is alive.
        verify(!processor.worldLink.requested() && !processor.worldLink.online(),
               "SkellWorld network connection must be off by default");
        verify(processor.worldLink.join(),"Opt-in local WorldLink worker failed to start");
        verify(processor.worldLink.requested(),"Opt-in state not observable");
        processor.worldLink.leave();
        verify(!processor.worldLink.requested() && !processor.worldLink.online(),
               "Leaving WorldLink did not release the connection");


        // Shared SkellCore captures the ACTUAL editor child tree at every
        // supported size, across all five modes, with both dry and morphed settings.
        skellcore::ui::headless::Options options;
        options.project="Skell-Flang v1.0 hardware mixer - four dial layout";
        options.minimumColours=12;
        for(const int width:std::array<int,4>{540,720,900,1080})
            for(size_t mode=0;mode<choices.size();++mode)
                for(int morphed=0;morphed<2;++morphed) {
                    skellcore::ui::headless::Scene scene;
                    scene.id="Skell-Flang-"+juce::String(width)+"-"+
                              juce::String(names[mode]).toLowerCase()+
                              (morphed?"-morphed":"-unmorphed");
                    scene.width=width;
                    scene.height=juce::roundToInt(double(width)*500./720.);
                    scene.mode=choices[mode];
                    scene.browser=morphed!=0;
                    options.scenes.push_back(scene);
                }
        // Also capture all five selected effects in both additional true
        // themes at a comfortable size; guard against tint-only theme changes.
        for(size_t mode=0;mode<choices.size();++mode)
            for(int skin=1;skin<=2;++skin) {
                skellcore::ui::headless::Scene scene;
                scene.id="Skell-Flang-720-"+juce::String(names[mode]).toLowerCase()+
                        (skin==1?"-graphite":"-bone");
                scene.width=720;
                scene.height=500;
                scene.mode=choices[mode];
                scene.browser=false;
                options.scenes.push_back(scene);
            }
        // Visual coverage for all four contextual macros in every effect.
        // These screenshots capture a real renderer image, not a mockup.
        for(size_t mode=0;mode<choices.size();++mode)
            for(int macro=0;macro<4;++macro) {
                skellcore::ui::headless::Scene scene;
                scene.id="Skell-Flang-720-"+juce::String(names[mode]).toLowerCase()+
                         "-macro"+juce::String(macro);
                scene.width=720;scene.height=500;
                scene.mode=choices[mode];
                scene.browser=false;
                options.scenes.push_back(scene);
            }
        if(!full) {
            // Four genuine rendered states: default, alternate effect, active,
            // and one of the plugin's real physical colour skins.
            options.scenes = {
                {"Skell-Flang-720-chorus-unmorphed",720,500,0,0,false},
                {"Skell-Flang-720-flanger-unmorphed",720,500,0,1,false},
                {"Skell-Flang-720-chorus-morphed",720,500,0,0,true},
                {"Skell-Flang-720-chorus-graphite",720,500,0,0,false}
            };
        }
        options.configure=[&](const auto& scene) {
            auto* tab=findButton(*editor,juce::String(
                scene.mode==0?"CHORUS":scene.mode==1?"FLANGER":
                scene.mode==6?"FLANGUS":scene.mode==4?"ORBIT":"DIMENSION"));
            click(tab,"Mode screenshot selector could not be clicked");
            const int expectedTheme=scene.id.endsWith("-graphite")?1:
                                    scene.id.endsWith("-bone")?2:0;
            for(int i=0;i<3 && look->themeIndex!=expectedTheme;++i)
                click(skull,"Cannot switch editor theme for renderer snapshot");
            verify(look->themeIndex==expectedTheme,"Wrong theme in screenshot");
            const juce::Colour identity=scene.mode==0?juce::Colour::fromRGB(75,230,172):
                scene.mode==1?juce::Colour::fromRGB(60,203,245):
                scene.mode==6?juce::Colour::fromRGB(154,244,77):
                scene.mode==4?juce::Colour::fromRGB(138,160,255):
                               juce::Colour::fromRGB(246,202,131);
            verify(look->accent==identity,"Theme altered the selected effect color");
            verify(int(processor.parameters.getRawParameterValue("mode")->load())==scene.mode,
                   "Wrong mode in screenshot");
            // Default scenes show macro 0. Dedicated character scenes select
            // a specific macro and verify that switching modes did not expose
            // dormant controls.
            int expectedMacro=0;
            if(scene.id.contains("-macro"))
                expectedMacro=juce::jlimit(0,3,
                    scene.id.fromLastOccurrenceOf("-macro",false,false).getIntValue());
            click(macroSelectors[size_t(expectedMacro)],
                  "Failed to select a renderer macro");
            verify(macroDials[size_t(expectedMacro)]->isVisible(),
                   "Renderer selected a hidden macro dial");
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
            int shownDials=0;
            for(auto* dial:macroDials)if(dial->isVisible())++shownDials;
            if(shownDials!=1)errors.add("Character dial count must equal one");
            for(auto* btn:macroSelectors)
                if(!btn->isVisible())errors.add("Character selector hidden");
            if(dest->getSelectedId()==0)
                errors.add("Morph destination is blank in a current-mode screenshot");
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
            {"Skell-Flang-540-orbit-unmorphed","Skell-Flang-540-dimension-unmorphed"},
            {"Skell-Flang-720-chorus-unmorphed","Skell-Flang-720-chorus-graphite"},
            {"Skell-Flang-720-chorus-graphite","Skell-Flang-720-chorus-bone"},
            {"Skell-Flang-720-flangus-macro0","Skell-Flang-720-flangus-macro3"}
        };
        if(!full) {
            options.mustDiffer = {
              {"Skell-Flang-720-chorus-unmorphed","Skell-Flang-720-flanger-unmorphed"},
              {"Skell-Flang-720-chorus-unmorphed","Skell-Flang-720-chorus-morphed"},
              {"Skell-Flang-720-chorus-unmorphed","Skell-Flang-720-chorus-graphite"}
            };
        }
        juce::Image probe(juce::Image::ARGB,8,8,true);
        {juce::Graphics g(probe);g.fillAll(juce::Colours::red);}
        verify(probe.getPixelAt(4,4).getRed()>200,
               "CI runner software rendering unavailable");

        const auto output=juce::File::isAbsolutePath(capturePath)
            ? juce::File(capturePath)
            : juce::File::getCurrentWorkingDirectory().getChildFile(capturePath);
        const auto report=skellcore::ui::headless::render(*editor,output,options);
        for(const auto& error:report.errors)
            std::cerr<<"Visual regression: "<<error.toStdString()<<"\n";
        verify(report.passed(),"Shared SkellCore visual regression failed");
        verify(report.captured==(full?70:4),"Wrong screenshot count for selected mode");
        if(!full) {
            const std::array<const char*,4> names{{
                "Skell-Flang-720-chorus-unmorphed",
                "Skell-Flang-720-flanger-unmorphed",
                "Skell-Flang-720-chorus-morphed",
                "Skell-Flang-720-chorus-graphite"}};
            constexpr int w=600,h=420;
            juce::Image sheet(juce::Image::RGB,2*w,2*h,true);
            juce::Graphics g(sheet);
            g.fillAll(juce::Colour(0xff070b0f));
            for(size_t i=0;i<names.size();++i) {
                const auto image=juce::ImageFileFormat::loadFrom(
                    output.getChildFile(juce::String(names[i])+".png"));
                verify(image.isValid(),"Missing real Flang scene for Quick4 sheet");
                g.drawImageWithin(image,int(i%2)*w+7,int(i/2)*h+7,
                                  w-14,h-14,juce::RectanglePlacement::centred);
            }
            verify(skellcore::ui::headless::writePng(
                       sheet,output.getChildFile("contact-sheet.png")),
                   "Could not save Flang Quick4 overview");
        }
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
        // Host-level regression: restore complete VST3 state and confirm
        // factory mode, preset name and master controls survive save/reload.
        processor.applyFactoryPreset(9);
        juce::MemoryBlock session;
        processor.getStateInformation(session);
        verify(session.getSize()>100,"VST3 session state was not serialized");
        processor.applyFactoryPreset(0);
        verify(int(processor.parameters.getRawParameterValue("mode")->load())==0,
               "State fixture did not reset to Chorus");
        processor.setStateInformation(session.getData(),int(session.getSize()));
        verify(int(processor.parameters.getRawParameterValue("mode")->load())==6,
               "Restored session lost Flangus mode");
        verify(processor.parameters.state.getProperty("presetName").toString()=="Flangus Jet",
               "Restored session lost preset identity");

        // Real processor mono/stereo/variable-block smoke at four host rates;
        // test abrupt mode changes, crossfeed, feedback and morph automation.
        const std::array<int,5> visibleModeIds{0,1,6,4,7};
        const std::array<int,5> blockSizes{1,17,64,257,512};
        for(const double sampleRate:std::array<double,4>{44100.,48000.,96000.,192000.}) {
            processor.prepareToPlay(sampleRate,512);
            verify(processor.getLatencySamples()==0,
                   "Sample-rate change broke declared zero-latency contract");
            for(const int channels:std::array<int,2>{1,2})
                for(const int samples:blockSizes)
                    for(const int selectedMode:visibleModeIds) {
                        auto* modeParameter=processor.parameters.getParameter("mode");
                        modeParameter->setValueNotifyingHost(
                            modeParameter->convertTo0to1(float(selectedMode)));
                        auto* feedback=processor.parameters.getParameter("feedback");
                        feedback->setValueNotifyingHost(
                            feedback->convertTo0to1(selectedMode==1?-.87f:.81f));
                        auto* morphParameter=processor.parameters.getParameter("morph");
                        morphParameter->setValueNotifyingHost(
                            morphParameter->convertTo0to1(selectedMode==4?.57f:.0f));
                        juce::AudioBuffer<float> hostAudio(channels,samples);
                        juce::MidiBuffer noMidi;
                        for(int block=0;block<5;++block) {
                            for(int ch=0;ch<channels;++ch)
                                for(int i=0;i<samples;++i) {
                                    const double tick=double(block*samples+i);
                                    const float x=float(.22*std::sin(2.*skellflang::pi*
                                        (ch?347.:173.)*tick/sampleRate));
                                    hostAudio.setSample(ch,i,x);
                                }
                            processor.processBlock(hostAudio,noMidi);
                            for(int ch=0;ch<channels;++ch)
                                for(int i=0;i<samples;++i) {
                                    const auto y=hostAudio.getSample(ch,i);
                                    verify(std::isfinite(y),
                                           "Non-finite output under host-rate/block automation");
                                    verify(std::abs(y)<=4.f,
                                           "Unsafe gain under host-rate/block automation");
                                }
                        }
                    }
            processor.releaseResources();
        }

        // The audio bypass must return exactly the supplied signal after its
        // finite smoothing ramp; this also protects saved DAW dry headroom.
        processor.prepareToPlay(48000.,256);
        if(auto* bypass=processor.parameters.getParameter("power"))
            bypass->setValueNotifyingHost(bypass->convertTo0to1(0.f));
        juce::MidiBuffer noMidi;
        juce::AudioBuffer<float> audioForBypass(2,256);
        for(int block=0;block<14;++block) {
            for(int i=0;i<256;++i) {
                const float sample=float(.27*std::sin(2.*skellflang::pi*
                    330.*(block*256+i)/48000.));
                audioForBypass.setSample(0,i,sample);
                audioForBypass.setSample(1,i,-sample);
            }
            processor.processBlock(audioForBypass,noMidi);
            if(block==13)for(int i=0;i<256;++i) {
                const float expected=float(.27*std::sin(2.*skellflang::pi*
                    330.*(block*256+i)/48000.));
                verify(std::abs(audioForBypass.getSample(0,i)-expected)<1.e-6f,
                       "Effect bypass altered left input");
                verify(std::abs(audioForBypass.getSample(1,i)+expected)<1.e-6f,
                       "Effect bypass altered right input");
            }
        }
        processor.releaseResources();
        return 0;
    }catch(const std::exception& e){
        std::cerr<<"Skell-Flang v0.9 UI test failed: "<<e.what()<<"\n";
        return 1;
    }
}
