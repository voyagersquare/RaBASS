#include "PluginProcessor.h"
#include <cstdlib>
#include <iostream>
#include <vector>

static void require(bool condition,const char* message)
{
    if (!condition) { std::cerr << "FAIL: " << message << '\n'; std::exit(1); }
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
        require(controls.size()==23,"all synth, preset and keyboard controls are present");
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
