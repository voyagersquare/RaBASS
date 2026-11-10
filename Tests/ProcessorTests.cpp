#include "PluginProcessor.h"
#include <cstdlib>
#include <iostream>
#include <vector>

static void require(bool condition,const char* message)
{
    if (!condition) { std::cerr << "FAIL: " << message << '\n'; std::exit(1); }
}
static void checkCutoffControl(RaBASSProcessor& processor,juce::Slider& slider)
{
    struct Listener final : juce::AudioProcessorParameter::Listener
    {
        int begins=0,ends=0;
        void parameterValueChanged(int,float) override {}
        void parameterGestureChanged(int,bool starting) override { if (starting) ++begins; else ++ends; }
    } listener;
    auto* parameter=processor.parameters.getParameter("cutoff");
    parameter->addListener(&listener);
    juce::int64 eventTime=10000;
    const auto event=[&](juce::Point<float> position,juce::ModifierKeys mods)
    {
        const juce::Time time(++eventTime);
        return juce::MouseEvent(juce::Desktop::getInstance().getMainMouseSource(),position,mods,
            1,0,0,0,0,&slider,&slider,time,{40,40},time,1,true);
    };
    const auto reset=[&](float hz)
    {
        parameter->setValueNotifyingHost(parameter->convertTo0to1(hz));
        listener.begins=listener.ends=0;
    };
    for (float hz : {200.0f,700.0f,5000.0f})
    {
        reset(hz);
        slider.mouseDown(event({40,40},juce::ModifierKeys::leftButtonModifier));
        slider.mouseDrag(event({40,39},juce::ModifierKeys::leftButtonModifier));
        slider.mouseUp(event({40,39},{}));
        const double delta=slider.getValue()-hz;
        require(delta>0 && delta<hz*.015,"one-pixel cutoff drag makes a gradual change across the frequency range");
        require(std::abs(slider.getValue()-processor.parameters.getRawParameterValue("cutoff")->load())<.01,
            "gradual drag updates the audio parameter");
        require(listener.begins==1 && listener.ends==1,"cutoff drag keeps balanced host automation gestures");
    }
    juce::MouseWheelDetails wheel{0,.1f,false,false,false};
    reset(700);
    slider.mouseWheelMove(event({40,40},{}),wheel);
    const double normalStep=slider.getValue()-700;
    require(normalStep>0 && normalStep<15,"cutoff wheel makes small frequency changes");
    require(listener.begins==1 && listener.ends==1,"cutoff wheel keeps balanced host automation gestures");
    reset(700);
    slider.mouseWheelMove(event({40,40},juce::ModifierKeys::shiftModifier),wheel);
    require(slider.getValue()>700 && slider.getValue()-700<normalStep*.11,"Shift wheel provides extra cutoff precision");
    wheel.isReversed=true;
    reset(700);
    slider.mouseWheelMove(event({40,40},{}),wheel);
    require(slider.getValue()<700,"cutoff respects reversed wheel direction");
    reset(700);
    require(slider.keyPressed(juce::KeyPress(juce::KeyPress::upKey)),"cutoff accepts arrow adjustments");
    require(std::abs(slider.getValue()-701)<.01,"cutoff arrow step is one Hz");
    require(slider.keyPressed(juce::KeyPress(juce::KeyPress::downKey,juce::ModifierKeys::shiftModifier,0)),
        "cutoff accepts Shift arrow adjustments");
    require(std::abs(slider.getValue()-700.9)<.01,"Shift arrow gives tenth-Hz cutoff precision");
    require(listener.begins==2 && listener.ends==2,"cutoff keyboard changes report complete automation gestures");
    require(slider.getTextFromValue(1000.1)!=slider.getTextFromValue(1000.2),"cutoff readout shows sub-Hz adjustments above one kHz");
    require(std::abs(slider.getValueFromText("1.234 kHz")-1234)<.01,"manual cutoff entry accepts kHz");
    reset(18000);
    slider.mouseWheelMove(event({40,40},{}),juce::MouseWheelDetails{0,.1f,false,false,false});
    require(slider.getValue()==18000,"fine cutoff cannot exceed its maximum");
    reset(30);
    slider.keyPressed(juce::KeyPress(juce::KeyPress::downKey));
    require(slider.getValue()==30,"fine cutoff cannot fall below its minimum");
    parameter->removeListener(&listener);
    reset(700);
}
int main(int argc,char** argv)
{
    juce::ScopedJuceInitialiser_GUI gui;
    RaBASSProcessor p;
    require(p.acceptsMidi() && !p.producesMidi(),"instrument MIDI routing");
    p.prepareToPlay(48000,512);
    juce::AudioBuffer<float> audio(2,512);
    juce::MidiBuffer midi;
    midi.addEvent(juce::MidiMessage::noteOn(1,36,1.0f),100);
    p.processBlock(audio,midi);
    require(audio.getMagnitude(0,100)==0,"sample accurate MIDI onset");
    require(audio.getMagnitude(100,412)>.001f,"instrument output");
    require(midi.isEmpty(),"no MIDI output");
    p.setCurrentProgram(3);
    auto* cutoff=p.parameters.getParameter("cutoff");
    cutoff->setValueNotifyingHost(cutoff->convertTo0to1(321.0f));
    const std::array<float,3> levels {.2f,.6f,.9f};
    for (size_t i=0;i<levels.size();++i)
    {
        auto* level=p.parameters.getParameter(parameterIds[16+i]);
        level->setValueNotifyingHost(level->convertTo0to1(levels[i]));
    }
    juce::MemoryBlock saved;
    p.getStateInformation(saved);
    RaBASSProcessor restored;
    restored.setStateInformation(saved.getData(),static_cast<int>(saved.getSize()));
    require(std::abs(restored.parameters.getRawParameterValue("cutoff")->load()-321)<.01f,"DAW project parameter restore");
    require(restored.getCurrentProgram()==3,"DAW project program restore");
    for (size_t i=0;i<levels.size();++i)
        require(std::abs(restored.parameters.getRawParameterValue(parameterIds[16+i])->load()-levels[i])<.0001f,"oscillator mix restored with DAW project");
    const char invalid[]="invalid data";
    restored.setStateInformation(invalid,sizeof(invalid));
    require(restored.getCurrentProgram()==3,"invalid state ignored");
    for (int shape=0;shape<3;++shape)
    {
        auto legacy=p.parameters.copyState();
        legacy.setProperty("schemaVersion",1,nullptr);
        legacy.getChildWithProperty("id","wave").setProperty("value",shape,nullptr);
        for (int i=16;i<19;++i)
            legacy.removeChild(legacy.getChildWithProperty("id",parameterIds[static_cast<size_t>(i)]),nullptr);
        juce::MemoryBlock oldState;
        juce::AudioProcessor::copyXmlToBinary(*legacy.createXml(),oldState);
        restored.setStateInformation(oldState.getData(),static_cast<int>(oldState.getSize()));
        require(std::abs(restored.parameters.getRawParameterValue("cutoff")->load()-321)<.01f,"legacy session keeps filter settings");
        const auto saw=restored.parameters.getRawParameterValue("sawLevel")->load();
        const auto triangle=restored.parameters.getRawParameterValue("triangleLevel")->load();
        const auto sine=restored.parameters.getRawParameterValue("sineLevel")->load();
        require(std::abs(saw-(shape==2 ? 0.0f : shape==1 ? .7f : 1.0f))<.0001f,"legacy saw or pulse selection migrated");
        require(std::abs(triangle-(shape==2 ? 1.0f : shape==1 ? .3f : 0.0f))<.0001f && sine<.0001f,"legacy triangle selection migrated");
    }
    for (int i=0;i<p.getNumPrograms();++i)
    {
        p.setCurrentProgram(i); midi.clear();
        midi.addEvent(juce::MidiMessage::noteOn(1,36,.9f),0);
        p.processBlock(audio,midi);
        require(audio.getMagnitude(0,512)>.00001f,"all factory presets produce sound");
    }
    p.reset(); p.processBlock(audio,midi);
    require(audio.getMagnitude(0,512)==0,"host reset clears sound");
    p.setCurrentProgram(0);
    for (int i=0;i<3;++i)
    {
        std::unique_ptr<juce::AudioProcessorEditor> editor(p.createEditor());
        require(editor && editor->getWidth()==1140 && editor->getHeight()==760,"preview editor opens and reopens");
        std::vector<juce::Component*> controls;
        std::vector<juce::Component*> labels;
        for (auto* child : editor->getChildren())
        {
            if (!child->isVisible()) continue;
            if (dynamic_cast<juce::Slider*>(child) || dynamic_cast<juce::ComboBox*>(child)
                || dynamic_cast<juce::TextButton*>(child) || dynamic_cast<juce::MidiKeyboardComponent*>(child))
                controls.push_back(child);
            else if (dynamic_cast<juce::Label*>(child)) labels.push_back(child);
        }
        require(controls.size()==24,"all synth, save-preset and keyboard controls are present");
        if (i==0)
        {
            juce::Slider* cutoffSlider=nullptr;
            for (auto* control : controls)
                if (control->getName()=="CUTOFF") cutoffSlider=dynamic_cast<juce::Slider*>(control);
            require(cutoffSlider!=nullptr,"cutoff control is present");
            checkCutoffControl(p,*cutoffSlider);
        }
        for (size_t control=0;control<controls.size();++control)
        {
            require(editor->getLocalBounds().reduced(16).contains(controls[control]->getBounds()),"control stays inside the editor");
            for (size_t other=control+1;other<controls.size();++other)
                require(!controls[control]->getBounds().intersects(controls[other]->getBounds()),"interactive controls never overlap");
            for (auto* label : labels)
                require(!controls[control]->getBounds().intersects(label->getBounds()),"labels do not cover interactive controls");
        }
        if (i==0 && argc>1)
        {
            auto image=editor->createComponentSnapshot(editor->getLocalBounds(),true,1.0f);
            juce::File output=juce::File::getCurrentWorkingDirectory().getChildFile(argv[1]);
            output.deleteFile();
            auto stream=output.createOutputStream();
            require(stream!=nullptr,"screenshot file");
            require(juce::PNGImageFormat{}.writeImageToStream(image,*stream),"editor screenshot");
        }
    }
    // Mono output is accepted; there is no audio input bus.
    auto layout=p.getBusesLayout(); layout.outputBuses.set(0,juce::AudioChannelSet::mono());
    require(p.setBusesLayout(layout),"mono bus supported");
    p.prepareToPlay(44100,256); audio.setSize(1,256);
    midi.addEvent(juce::MidiMessage::noteOn(1,40,1.0f),0); p.processBlock(audio,midi);
    require(audio.getMagnitude(0,256)>.0001f,"mono audio render");
    std::cout << "PASS: processor, state, presets, editor lifecycle, mono and stereo buses\n";
}
