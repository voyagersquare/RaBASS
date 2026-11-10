#pragma once
#include "PluginProcessor.h"

class BassLookAndFeel final : public juce::LookAndFeel_V4
{
public:
    BassLookAndFeel();
    void drawRotarySlider(juce::Graphics&,int,int,int,int,float,float,float,juce::Slider&) override;
};
class RaBASSEditor final : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit RaBASSEditor(RaBASSProcessor&);
    ~RaBASSEditor() override;
    void paint(juce::Graphics&) override;
    void resized() override;
private:
    struct Dial
    {
        juce::Slider slider;
        juce::Label label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    };
    void timerCallback() override;
    RaBASSProcessor& processor;
    BassLookAndFeel look;
    std::array<Dial,14> dials;
    std::array<Dial,3> oscillatorLevels;
    juce::ComboBox mode, presets;
    juce::Label modeLabel, presetLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> modeAttachment;
    juce::MidiKeyboardComponent keyboard;
    juce::TextButton panic{"PANIC"};
    float displayedPeak=0;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RaBASSEditor)
};
