#pragma once
#include <JuceHeader.h>
#include "BassEngine.h"
#include <vector>

inline constexpr std::array<const char*,19> parameterIds {
    "wave", "mode", "mix", "detune", "sub", "cutoff", "resonance", "filterEnv", "filterDecay",
    "attack", "decay", "sustain", "release", "drive", "glide", "output",
    "sawLevel", "triangleLevel", "sineLevel"
};

class RaBASSProcessor final : public juce::AudioProcessor
{
public:
    struct UserPreset { juce::String name; juce::File file; };
    explicit RaBASSProcessor(juce::File presetFolder = {});
    static juce::File defaultUserPresetFolder();
    juce::File getUserPresetFolder() const { return userPresetFolder; }
    std::vector<UserPreset> getUserPresets() const;
    bool userPresetExists(const juce::String&) const;
    juce::Result saveUserPreset(const juce::String&, bool replaceExisting = false);
    juce::Result loadUserPreset(const juce::File&);
    juce::String getCurrentUserPresetName() const;
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
    static bool isValidUserPresetName(const juce::String&);
    juce::Result readUserPreset(const juce::File&,juce::ValueTree&,juce::String&) const;
    void setUserPresetName(const juce::String&);
    std::array<std::atomic<float>*,19> raw{};
    rabass::BassEngine engine;
    std::atomic<int> currentProgram{0};
    std::atomic<bool> resetRequested{false};
    const juce::File userPresetFolder;
    mutable juce::CriticalSection presetNameLock;
    juce::String currentUserPresetName;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RaBASSProcessor)
};
