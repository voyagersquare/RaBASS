#pragma once
#include "PluginProcessor.h"

class BassLookAndFeel final : public juce::LookAndFeel_V4
{
public:
    BassLookAndFeel();
    void drawRotarySlider(juce::Graphics&,int,int,int,int,float,float,float,juce::Slider&) override;
    void drawLinearSlider(juce::Graphics&,int,int,int,int,float,float,float,juce::Slider::SliderStyle,juce::Slider&) override;
    void drawComboBox(juce::Graphics&,int,int,bool,int,int,int,int,juce::ComboBox&) override;
    void drawButtonBackground(juce::Graphics&,juce::Button&,const juce::Colour&,bool,bool) override;
    void drawButtonText(juce::Graphics&,juce::TextButton&,bool,bool) override;
    juce::Label* createSliderTextBox(juce::Slider&) override;
};
class PresetComboBox final : public juce::ComboBox
{
public:
    std::function<void()> beforePopup;
    void showPopup() override { if (beforePopup) beforePopup(); juce::ComboBox::showPopup(); }
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
    void refreshPresetMenu();
    void syncPresetSelection();
    void navigatePreset(int direction);
    void beginSavePreset();
    void saveNamedPreset(const juce::String&,bool replaceExisting);
    void showPresetError(const juce::String&);
    juce::AlertWindow* makePresetDialog(const juce::String&,const juce::String&);
    RaBASSProcessor& processor;
    BassLookAndFeel look;
    std::array<Dial,14> dials;
    std::array<Dial,3> oscillatorLevels;
    juce::ComboBox mode;
    PresetComboBox presets;
    std::vector<RaBASSProcessor::UserPreset> userPresets;
    juce::Label modeLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> modeAttachment;
    juce::MidiKeyboardComponent keyboard;
    juce::TextButton panic{"PANIC"};
    juce::TextButton previousPreset{"<"}, nextPreset{">"};
    juce::TextButton savePreset{"SAVE PRESET"};
    juce::Component::SafePointer<juce::AlertWindow> presetDialog;
    juce::Image metalSurface;
    float displayedPeak=0;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RaBASSEditor)
};
