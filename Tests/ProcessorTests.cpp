#include "PluginProcessor.h"
#include <cstdlib>
#include <iostream>

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
    juce::MemoryBlock saved;
    p.getStateInformation(saved);
    RaBASSProcessor restored;
    restored.setStateInformation(saved.getData(),static_cast<int>(saved.getSize()));
    require(std::abs(restored.parameters.getRawParameterValue("cutoff")->load()-321)<.01f,"DAW project parameter restore");
    require(restored.getCurrentProgram()==3,"DAW project program restore");
    const char invalid[]="invalid data";
    restored.setStateInformation(invalid,sizeof(invalid));
    require(restored.getCurrentProgram()==3,"invalid state ignored");
    for (int i=0;i<p.getNumPrograms();++i)
    {
        p.setCurrentProgram(i); midi.clear();
        midi.addEvent(juce::MidiMessage::noteOn(1,36,.9f),0);
        p.processBlock(audio,midi);
        require(audio.getMagnitude(0,512)>.00001f,"all factory presets produce sound");
    }
    p.reset(); p.processBlock(audio,midi);
    require(audio.getMagnitude(0,512)==0,"host reset clears sound");
    for (int i=0;i<3;++i)
    {
        std::unique_ptr<juce::AudioProcessorEditor> editor(p.createEditor());
        require(editor && editor->getWidth()==1000 && editor->getHeight()==640,"editor opens and reopens");
        if (i==0 && argc>1)
        {
            auto image=editor->createComponentSnapshot(editor->getLocalBounds(),true,1.0f);
            juce::File output=juce::File::getCurrentWorkingDirectory().getChildFile(argv[1]);
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
