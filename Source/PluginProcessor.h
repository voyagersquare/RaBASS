#pragma once
#include <JuceHeader.h>
#include "BassEngine.h"

inline constexpr std::array<const char*,16> parameterIds {
    "wave", "mode", "mix", "detune", "sub", "cutoff", "resonance", "filterEnv", "filterDecay",
    "attack", "decay", "sustain", "release", "drive", "glide", "output"
};

class RaBASSProcessor final : public juce::AudioProcessor
{
public:
    RaBASSProcessor();
    void prepareToPlay(double, int) override;
    void releaseResources() override;
    void reset() override;
    bool isBusesLayoutSupported(const BusesLayout&) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 5.0; }
    int getNumPrograms() override;
    int getCurrentProgram() override { return currentProgram.load(); }
    void setCurrentProgram(int) override;
    const juce::String getProgramName(int) override;
    void changeProgramName(int, const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;
    juce::AudioProcessorValueTreeState parameters;
    juce::MidiKeyboardState keyboard;
    std::atomic<float> peak { 0 };
private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    rabass::Parameters readParameters() const noexcept;
    void dispatchMidi(const juce::MidiMessage&);
    std::array<std::atomic<float>*,16> raw{};
    rabass::BassEngine engine;
    std::atomic<int> currentProgram{0};
    std::atomic<bool> resetRequested{false};
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RaBASSProcessor)
};
