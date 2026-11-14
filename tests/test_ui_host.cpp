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
        verify(preset->getWidth()>125 && preset->getRight()<cog->getX(),
               "Sound library selector must be legible and clear of settings");
        for(const auto* name:names) {
            auto* tab=findButton(*editor,name);
            verify(tab && tab->getHeight()<=24 && tab->getWidth()<=85,
                   "Effect mode selector is no longer the compact design");
        }
        verify(byID(*editor,"mod")==nullptr && findButton(*editor,"MOD")==nullptr &&
               findButton(*editor,"BACK")==nullptr,
               "Obsolete MOD drawer button is still present");

        auto* wave=dynamic_cast<juce::ComboBox*>(editor->findChildWithID("lfo-wave"));
        auto* sync=byID(*editor,"lfo-sync");
        verify(wave&&sync,"Missing WAVE or SYNC");
        verify(editor->findChildWithID("morph")==nullptr &&
               editor->findChildWithID("morph-target")==nullptr,
               "Retired TO/MORPH controls are still visible");
        verify(wave->getX()>390&&wave->getY()>200&&wave->getBottom()<260,
               "WAVE must fit beneath the signal monitor");
        verify(sync->getY()<65&&sync->getRight()<cog->getX()&&
               sync->getButtonText().isEmpty(),
               "SYNC symbol must be before settings");
        processor.applyFactoryPreset(0);
        auto* chorus=findButton(*editor,"CHORUS");
        auto* flanger=findButton(*editor,"FLANGER");
        auto* orbit=findButton(*editor,"ORBIT");
        auto* dimension=findButton(*editor,"DIMENSION");
        click(flanger,"Could not enable a second layer");
        verify(processor.parameters.getRawParameterValue("layerFlanger")->load()>.5f &&
               processor.parameters.getRawParameterValue("layerChorus")->load()>.5f,
               "Enabling Flanger must not remove Chorus");
        click(orbit,"Cannot add Orbit layer");
        click(dimension,"Cannot add Dimension layer");
        verify(processor.parameters.getRawParameterValue("layerOrbit")->load()>.5f &&
               processor.parameters.getRawParameterValue("layerDimension")->load()>.5f,
               "Multiple simultaneous layers not active");
        click(orbit,"Cannot disable Orbit");
        verify(processor.parameters.getRawParameterValue("layerOrbit")->load()<.5f,
               "Clicking Orbit again must disable it");
        click(dimension,"Cannot disable Dimension");
        click(flanger,"Cannot disable Flanger");
        click(chorus,"Last active layer must be protected");
        verify(processor.parameters.getRawParameterValue("layerChorus")->load()>.5f,
               "All-off click is not guarded");
        processor.applyFactoryPreset(0);
        auto* look=dynamic_cast<FlangLookAndFeel*>(&editor->getLookAndFeel());
        verify(look!=nullptr,"Skell-Flang is not using the custom look-and-feel");
        // Factory presets are host-driven; refresh presentation before
        // comparing the independent theme and focused effect identity.
        for(int i=0;i<3;++i)click(skull,"Cannot refresh preset focus");
        const auto initialAccent=look->accent;
        const auto initialBackground=look->background;
        click(skull,"Skull theme cycle is not clickable");
        verify(look->background!=initialBackground,
               "Skull click did not actually change the interface background");
        verify(look->accent==initialAccent,
               "Skull theme click incorrectly changed the effect identity accent");
        verify(wave->isVisible(),"Skull theme changed WAVE visibility");
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
                              (morphed?"-stacked":"-solo");
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
                {"Skell-Flang-720-chorus-solo",720,500,0,0,false},
                {"Skell-Flang-720-flanger-solo",720,500,0,1,false},
                {"Skell-Flang-720-chorus-stacked",720,500,0,0,true},
                {"Skell-Flang-720-chorus-graphite",720,500,0,0,false}
            };
        }
        options.configure=[&](const auto& scene) {
            const int desired=scene.mode;
            const int bank=desired==0?0:desired==1?4:desired==6?7:desired==4?10:13;
            processor.applyFactoryPreset(bank);
            if(scene.browser) {
                const int extra=desired==0?1:0;
                click(findButton(*editor,extra==0?"CHORUS":"FLANGER"),
                      "Second layer could not be enabled in renderer");
                if(auto* p=processor.parameters.getParameter("mode"))
                    p->setValueNotifyingHost(p->convertTo0to1(float(desired)));
            }
            // Three skull clicks force a refresh even when target theme is
            // already correct (the preset is applied by the host).
            for(int i=0;i<3;++i)
                click(skull,"Cannot refresh real JUCE editor");
            const int wanted=scene.id.endsWith("-graphite")?1:
                             scene.id.endsWith("-bone")?2:0;
            for(int i=0;i<3&&look->themeIndex!=wanted;++i)
                click(skull,"Cannot apply scene theme");
            verify(look->themeIndex==wanted,"Wrong screenshot theme");
            const juce::String expectedName=processor.parameters.state
                .getProperty("presetName","INIT").toString();
            verify(preset->getButtonText()==expectedName,
                   "Preset bar shows a stale factory/custom name");
            verify(scene.browser ? expectedName=="CUSTOM" : expectedName!="CUSTOM",
                   "Solo/stacked preset labels were not restored correctly");
            const juce::Colour tint=desired==0?juce::Colour::fromRGB(75,230,172):
                 desired==1?juce::Colour::fromRGB(60,203,245):
                 desired==6?juce::Colour::fromRGB(154,244,77):
                 desired==4?juce::Colour::fromRGB(138,160,255):
                              juce::Colour::fromRGB(246,202,131);
            verify(look->accent==tint,"Wrong focused effect accent");
            verify(int(processor.parameters.getRawParameterValue("mode")->load())==desired,
                   "Wrong focused mode in screenshot");
            int selectedMacro=0;
            if(scene.id.contains("-macro"))
                selectedMacro=juce::jlimit(0,3,scene.id.fromLastOccurrenceOf(
                    "-macro",false,false).getIntValue());
            click(macroSelectors[size_t(selectedMacro)],"Cannot select macro");
            verify(macroDials[size_t(selectedMacro)]->isVisible(),
                   "Wrong macro dial visibility");
            verify(wave->isVisible()&&sync->isVisible(),
                   "Signal wave or header sync control hidden");
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
            if(count<23)errors.add("Too few visible controls");
            int shownDials=0;
            for(auto* dial:macroDials)if(dial->isVisible())++shownDials;
            if(shownDials!=1)errors.add("Character dial count must equal one");
            for(auto* btn:macroSelectors)
                if(!btn->isVisible())errors.add("Character selector hidden");
            if(wave->getSelectedId()==0)
                errors.add("Waveform selection is blank");
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
            {"Skell-Flang-540-chorus-solo","Skell-Flang-540-chorus-stacked"},
            {"Skell-Flang-540-flanger-solo","Skell-Flang-540-flangus-solo"},
            {"Skell-Flang-540-orbit-solo","Skell-Flang-540-dimension-solo"},
            {"Skell-Flang-720-chorus-solo","Skell-Flang-720-chorus-graphite"},
            {"Skell-Flang-720-chorus-graphite","Skell-Flang-720-chorus-bone"},
            {"Skell-Flang-720-flangus-macro0","Skell-Flang-720-flangus-macro3"}
        };
        if(!full) {
            options.mustDiffer = {
              {"Skell-Flang-720-chorus-solo","Skell-Flang-720-flanger-solo"},
              {"Skell-Flang-720-chorus-solo","Skell-Flang-720-chorus-stacked"},
              {"Skell-Flang-720-chorus-solo","Skell-Flang-720-chorus-graphite"}
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
                "Skell-Flang-720-chorus-solo",
                "Skell-Flang-720-flanger-solo",
                "Skell-Flang-720-chorus-stacked",
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

        if(auto* old=processor.parameters.getParameter("layerMode"))
            old->setValueNotifyingHost(old->convertTo0to1(0.f));
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

        // New layer flags must round-trip through a VST3 project state.
        processor.applyFactoryPreset(7); // Flangus Glow
        auto* orbitFlag=processor.parameters.getParameter("layerOrbit");
        orbitFlag->setValueNotifyingHost(orbitFlag->convertTo0to1(1.f));
        juce::MemoryBlock layerSession;
        processor.getStateInformation(layerSession);
        processor.applyFactoryPreset(0);
        processor.setStateInformation(layerSession.getData(),int(layerSession.getSize()));
        verify(processor.parameters.getRawParameterValue("layerMode")->load()>.5f &&
               processor.parameters.getRawParameterValue("layerFlangus")->load()>.5f &&
               processor.parameters.getRawParameterValue("layerOrbit")->load()>.5f,
               "Layer stack was not restored from saved FL Studio state");

        // Audio must actually differ with two independent engines, without
        // duplicated dry gain or nonfinite output. Compare identical inputs.
        const auto measure=[&](bool layeredFlanger) {
            processor.applyFactoryPreset(0);
            auto* flangerFlag=processor.parameters.getParameter("layerFlanger");
            flangerFlag->setValueNotifyingHost(
                flangerFlag->convertTo0to1(layeredFlanger?1.f:0.f));
            processor.prepareToPlay(48000.,128);
            juce::AudioBuffer<float> signal(2,128);
            juce::MidiBuffer none;
            std::array<float,512> tail{};
            for(int block=0;block<150;++block) {
                for(int i=0;i<128;++i) {
                    const double t=double(block*128+i)/48000.;
                    const float input=float(.21*std::sin(2.*skellflang::pi*173.*t)
                                           +.07*std::sin(2.*skellflang::pi*461.*t));
                    signal.setSample(0,i,input);
                    signal.setSample(1,i,-input);
                }
                processor.processBlock(signal,none);
                for(int i=0;i<128;++i) {
                    const float l=signal.getSample(0,i);
                    const float r=signal.getSample(1,i);
                    verify(std::isfinite(l)&&std::isfinite(r),
                           "Stacking created a non-finite audio sample");
                    verify(std::abs(l)<4.f&&std::abs(r)<4.f,
                           "Stacking exceeded safe normal headroom");
                    if(block>=146)
                        tail[size_t((block-146)*128+i)]=l;
                }
            }
            processor.releaseResources();
            return tail;
        };
        const auto solo=measure(false);
        const auto stacked=measure(true);
        double audibleDifference=0.;
        for(size_t i=0;i<solo.size();++i)
            audibleDifference+=std::abs(double(solo[i])-stacked[i]);
        verify(audibleDifference>.05,
               "Layer Stack enabled two flags but did not combine two engines");

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
