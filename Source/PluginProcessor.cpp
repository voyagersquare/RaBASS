#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
struct Preset { const char* name; std::array<float,19> values; };
constexpr std::array<Preset,6> presets {{
    {"01  Foundation", {0,0,.35f,7,.4f,700,.2f,3,.25f,.005f,.25f,.65f,.15f,.2f,.07f,-9, .75f,.2f,.15f}},
    {"02  Rubber",     {1,0,.25f,3,.3f,240,.55f,4.8f,.18f,.003f,.18f,.35f,.12f,.3f,.09f,-10, .25f,.65f,.1f}},
    {"03  Deep Sub",   {2,0,0,0,.95f,190,.05f,.4f,.5f,.012f,.4f,.85f,.3f,.08f,.12f,-6, 0,.15f,1}},
    {"04  Acid Line",  {0,0,.1f,1,.15f,450,.75f,5,.16f,.002f,.16f,.2f,.1f,.45f,.06f,-12, 1,0,0}},
    {"05  Detuned",    {0,0,.5f,18,.35f,1200,.3f,2,.4f,.008f,.4f,.7f,.22f,.35f,.15f,-11, .8f,.2f,.15f}},
    {"06  Velvet Poly",{2,1,.4f,9,.25f,1800,.1f,1.5f,.6f,.035f,.55f,.6f,.65f,.12f,0,-12, 0,.85f,.3f}}
}};
}

juce::AudioProcessorValueTreeState::ParameterLayout RaBASSProcessor::createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    // Retain the original ID and position for older DAW sessions. It is used
    // only when migrating their saved oscillator selection to the new mixer.
    layout.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"wave",1}, "Waveform", juce::StringArray{"Saw", "Pulse", "Triangle"},0,
        juce::AudioParameterChoiceAttributes{}.withAutomatable(false)));
    layout.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"mode",1}, "Voice mode", juce::StringArray{"Mono / legato", "Poly / 8 voices"},0));
    const auto add = [&layout](const char* id, const char* name, float low, float high, float value, float centre, const char* unit)
    {
        juce::NormalisableRange<float> range(low,high);
        if (centre>low && centre<high) range.setSkewForCentre(centre);
        layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{id,1},name,range,value,
            juce::AudioParameterFloatAttributes{}.withLabel(unit)));
    };
    add("mix","Oscillator blend",0,1,.35f,.5f,"");
    add("detune","Oscillator 2 detune",0,30,7,10,"ct");
    add("sub","Sub level",0,1,.4f,.5f,"");
    add("cutoff","Filter cutoff",30,18000,700,700,"Hz");
    add("resonance","Resonance",0,1,.2f,.5f,"");
    add("filterEnv","Filter envelope",0,6,3,3,"oct");
    add("filterDecay","Filter decay",.02f,3,.25f,.35f,"s");
    add("attack","Attack",.001f,2,.005f,.08f,"s");
    add("decay","Decay",.02f,3,.25f,.35f,"s");
    add("sustain","Sustain",0,1,.65f,.5f,"");
    add("release","Release",.01f,4,.15f,.4f,"s");
    add("drive","Drive",0,1,.2f,.5f,"");
    add("glide","Glide",0,.5f,.07f,.1f,"s");
    add("output","Output",-36,6,-9,-9,"dB");
    add("sawLevel","Sawtooth level",0,1,.75f,.5f,"");
    add("triangleLevel","Triangle level",0,1,.2f,.5f,"");
    add("sineLevel","Sine level",0,1,.15f,.5f,"");
    return layout;
}

RaBASSProcessor::RaBASSProcessor()
    : AudioProcessor(BusesProperties().withOutput("Output",juce::AudioChannelSet::stereo(),true)),
      parameters(*this,nullptr,"RaBASSState",createLayout())
{
    for (size_t i=0;i<raw.size();++i) raw[i]=parameters.getRawParameterValue(parameterIds[i]);
}
void RaBASSProcessor::prepareToPlay(double rate, int)
{
    engine.setParameters(readParameters());
    engine.prepare(rate);
    keyboard.reset(); peak=0;
}
void RaBASSProcessor::releaseResources() { resetRequested=true; peak=0; }
void RaBASSProcessor::reset() { resetRequested=true; }
bool RaBASSProcessor::isBusesLayoutSupported(const BusesLayout& layout) const
{
    return layout.getMainInputChannelSet().isDisabled() &&
        (layout.getMainOutputChannelSet()==juce::AudioChannelSet::mono() ||
         layout.getMainOutputChannelSet()==juce::AudioChannelSet::stereo());
}
rabass::Parameters RaBASSProcessor::readParameters() const noexcept
{
    std::array<float,19> v{};
    for (size_t i=0;i<v.size();++i) v[i]=raw[i]->load(std::memory_order_relaxed);
    rabass::Parameters p;
    p.poly=v[1]>.5f;
    p.mix=v[2]; p.detune=v[3]; p.sub=v[4]; p.cutoff=v[5]; p.resonance=v[6]; p.filterEnv=v[7]; p.filterDecay=v[8];
    p.attack=v[9]; p.decay=v[10]; p.sustain=v[11]; p.release=v[12]; p.drive=v[13]; p.glide=v[14]; p.outputDb=v[15];
    p.sawLevel=v[16]; p.triangleLevel=v[17]; p.sineLevel=v[18];
    return p;
}
void RaBASSProcessor::dispatchMidi(const juce::MidiMessage& m)
{
    const auto channel=m.getChannel();
    if (m.isNoteOn()) engine.noteOn(channel,m.getNoteNumber(),m.getFloatVelocity());
    else if (m.isNoteOff()) engine.noteOff(channel,m.getNoteNumber());
    else if (m.isPitchWheel()) engine.pitchWheel(channel,m.getPitchWheelValue());
    else if (m.isSustainPedalOn()) engine.sustainPedal(channel,true);
    else if (m.isSustainPedalOff()) engine.sustainPedal(channel,false);
    else if (m.isAllSoundOff()) engine.allNotesOff(channel,true);
    else if (m.isAllNotesOff()) engine.allNotesOff(channel,false);
    else if (m.isController() && m.getControllerNumber()==121)
    { engine.sustainPedal(channel,false); engine.pitchWheel(channel,8192); }
}
void RaBASSProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();
    if (resetRequested.exchange(false)) { engine.reset(); keyboard.reset(); }
    engine.setParameters(readParameters());
    const auto length=buffer.getNumSamples();
    if (buffer.getNumChannels()==0 || length==0) { midi.clear(); return; }
    keyboard.processNextMidiBuffer(midi,0,length,true);
    auto* left=buffer.getWritePointer(0);
    auto* right=buffer.getNumChannels()>1 ? buffer.getWritePointer(1) : nullptr;
    int position=0;
    for (const auto metadata : midi)
    {
        const auto offset=juce::jlimit(position,length,metadata.samplePosition);
        engine.render(left+position,right!=nullptr ? right+position : nullptr,offset-position);
        dispatchMidi(metadata.getMessage());
        position=offset;
    }
    engine.render(left+position,right!=nullptr ? right+position : nullptr,length-position);
    peak.store(buffer.getMagnitude(0,length),std::memory_order_relaxed);
    midi.clear();
}
int RaBASSProcessor::getNumPrograms() { return static_cast<int>(presets.size()); }
const juce::String RaBASSProcessor::getProgramName(int index)
{
    return juce::isPositiveAndBelow(index,getNumPrograms()) ? presets[static_cast<size_t>(index)].name : juce::String{};
}
void RaBASSProcessor::setCurrentProgram(int index)
{
    if (!juce::isPositiveAndBelow(index,getNumPrograms())) return;
    const auto& preset=presets[static_cast<size_t>(index)];
    for (size_t i=0;i<parameterIds.size();++i)
        if (auto* p=parameters.getParameter(parameterIds[i]))
        { p->beginChangeGesture(); p->setValueNotifyingHost(p->convertTo0to1(preset.values[i])); p->endChangeGesture(); }
    currentProgram=index;
    resetRequested=true;
}
void RaBASSProcessor::getStateInformation(juce::MemoryBlock& data)
{
    auto state=parameters.copyState();
    state.setProperty("schemaVersion",2,nullptr);
    state.setProperty("program",currentProgram.load(),nullptr);
    if (const auto xml=state.createXml()) copyXmlToBinary(*xml,data);
}
void RaBASSProcessor::setStateInformation(const void* data,int size)
{
    if (const auto xml=getXmlFromBinary(data,size))
    {
        if (!xml->hasTagName(parameters.state.getType().toString())) return;
        auto state=juce::ValueTree::fromXml(*xml);
        if (!state.isValid()) return;
        const int schema=static_cast<int>(state.getProperty("schemaVersion",1));
        if (schema<1 || schema>2) return;
        if (schema==1)
        {
            const auto legacyWave=state.getChildWithProperty("id","wave");
            const int shape=static_cast<int>(legacyWave.getProperty("value",0));
            // Pulse is approximated by a saw/triangle mix; the rest of the
            // old session's parameters, including detune and sub, are retained.
            const std::array<float,3> levels = shape==2 ? std::array<float,3>{0,1,0}
                : shape==1 ? std::array<float,3>{.7f,.3f,0} : std::array<float,3>{1,0,0};
            for (size_t i=0;i<levels.size();++i)
            {
                auto child=state.getChildWithProperty("id",parameterIds[16+i]);
                if (!child.isValid())
                { child=juce::ValueTree{"PARAM"}; child.setProperty("id",parameterIds[16+i],nullptr); state.addChild(child,-1,nullptr); }
                child.setProperty("value",levels[i],nullptr);
            }
            state.setProperty("schemaVersion",2,nullptr);
        }
        parameters.replaceState(state);
        currentProgram=juce::jlimit(0,getNumPrograms()-1,static_cast<int>(state.getProperty("program",0)));
        resetRequested=true;
    }
}
juce::AudioProcessorEditor* RaBASSProcessor::createEditor() { return new RaBASSEditor(*this); }
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new RaBASSProcessor(); }
