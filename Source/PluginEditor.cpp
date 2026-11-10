#include "PluginEditor.h"
namespace
{
const juce::Colour background{0xff0c1015}, panel{0xff151c24}, ink{0xffe5edf3}, muted{0xff8294a5}, accent{0xffa8ef70};
const juce::Colour amber{0xfff9b45a};
const std::array<const char*,14> labels {"BLEND","DETUNE","SUB","CUTOFF","RESONANCE","ENV AMT","ENV DECAY","ATTACK","DECAY","SUSTAIN","RELEASE","DRIVE","GLIDE","OUTPUT"};
}
BassLookAndFeel::BassLookAndFeel()
{
    setColour(juce::Slider::textBoxTextColourId,ink);
    setColour(juce::Slider::textBoxBackgroundColourId,juce::Colours::transparentBlack);
    setColour(juce::Slider::textBoxOutlineColourId,juce::Colours::transparentBlack);
    setColour(juce::ComboBox::backgroundColourId,juce::Colour{0xff202b36});
    setColour(juce::ComboBox::textColourId,ink);
    setColour(juce::ComboBox::outlineColourId,juce::Colour{0xff354553});
    setColour(juce::PopupMenu::backgroundColourId,panel);
    setColour(juce::PopupMenu::textColourId,ink);
    setColour(juce::PopupMenu::highlightedBackgroundColourId,juce::Colour{0xff324632});
    setColour(juce::TextButton::buttonColourId,juce::Colour{0xff202b36});
    setColour(juce::TextButton::textColourOffId,muted);
}
void BassLookAndFeel::drawRotarySlider(juce::Graphics& g,int x,int y,int width,int height,float value,float start,float end,juce::Slider& s)
{
    auto bounds=juce::Rectangle<float>(static_cast<float>(x),static_cast<float>(y),static_cast<float>(width),static_cast<float>(height)).reduced(9);
    const auto radius=juce::jmin(bounds.getWidth(),bounds.getHeight())*.5f;
    const auto centre=bounds.getCentre();
    juce::Path track, filled;
    track.addCentredArc(centre.x,centre.y,radius,radius,0,start,end,true);
    g.setColour(juce::Colour{0xff2e3c48}); g.strokePath(track,juce::PathStrokeType(3));
    const auto angle=start+value*(end-start);
    filled.addCentredArc(centre.x,centre.y,radius,radius,0,start,angle,true);
    g.setColour(s.findColour(juce::Slider::rotarySliderFillColourId)); g.strokePath(filled,juce::PathStrokeType(3));
    g.setColour(juce::Colour{0xff26323e}); g.fillEllipse(centre.x-radius+6,centre.y-radius+6,2*radius-12,2*radius-12);
    g.setColour(ink);
    g.drawLine(centre.x+std::sin(angle)*radius*.38f,centre.y-std::cos(angle)*radius*.38f,
               centre.x+std::sin(angle)*radius*.67f,centre.y-std::cos(angle)*radius*.67f,2);
}
RaBASSEditor::RaBASSEditor(RaBASSProcessor& p)
    : AudioProcessorEditor(p), processor(p), keyboard(p.keyboard,juce::MidiKeyboardComponent::horizontalKeyboard)
{
    setLookAndFeel(&look);
    setSize(1000,640);
    setResizable(false,false);
    for (size_t i=0;i<dials.size();++i)
    {
        auto& dial=dials[i];
        const juce::String id=parameterIds[i+2];
        dial.slider.setName(labels[i]);
        dial.slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        dial.slider.setTextBoxStyle(juce::Slider::TextBoxBelow,false,94,22);
        dial.slider.setColour(juce::Slider::rotarySliderFillColourId,i>=11 ? amber : accent);
        dial.slider.setColour(juce::Slider::textBoxTextColourId,ink);
        dial.slider.setColour(juce::Slider::textBoxBackgroundColourId,juce::Colours::transparentBlack);
        dial.slider.setColour(juce::Slider::textBoxOutlineColourId,juce::Colours::transparentBlack);
        dial.slider.setPopupDisplayEnabled(false,false,nullptr);
        dial.slider.setTooltip(processor.parameters.getParameter(id)->getName(64)+". Double-click to reset.");
        dial.attachment=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.parameters,id,dial.slider);
        dial.slider.textFromValueFunction=[id](double v)
        {
            if (id=="cutoff") return v>=1000 ? juce::String(v/1000,2)+" kHz" : juce::String(v,0)+" Hz";
            if (id=="detune") return juce::String(v,1)+" ct";
            if (id=="filterEnv") return juce::String(v,1)+" oct";
            if (id=="output") return juce::String(v,1)+" dB";
            if (id=="attack" || id=="decay" || id=="release" || id=="glide" || id=="filterDecay")
                return v<1 ? juce::String(v*1000,0)+" ms" : juce::String(v,2)+" s";
            return juce::String(v*100,0)+" %";
        };
        dial.slider.valueFromTextFunction=[id](const juce::String& text)
        {
            const auto value=text.getDoubleValue();
            if (text.containsIgnoreCase("kHz")) return value*1000;
            if (text.containsIgnoreCase("ms")) return value/1000;
            if (text.containsChar('%')) return value/100;
            if (id=="mix" || id=="sub" || id=="resonance" || id=="sustain" || id=="drive") return value/100;
            return value;
        };
        dial.slider.updateText();
        dial.label.setText(labels[i],juce::dontSendNotification);
        dial.label.setFont(juce::FontOptions(11.0f,juce::Font::bold));
        dial.label.setColour(juce::Label::textColourId,muted);
        dial.label.setJustificationType(juce::Justification::centred);
        addAndMakeVisible(dial.slider); addAndMakeVisible(dial.label);
    }
    waveform.addItemList({"Saw","Pulse","Triangle"},1);
    mode.addItemList({"Mono / legato","Poly / 8 voices"},1);
    for (int i=0;i<p.getNumPrograms();++i) presets.addItem(p.getProgramName(i),i+1);
    presets.setSelectedId(p.getCurrentProgram()+1,juce::dontSendNotification);
    presets.onChange=[this] { processor.setCurrentProgram(presets.getSelectedId()-1); };
    waveAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(p.parameters,"wave",waveform);
    modeAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(p.parameters,"mode",mode);
    addAndMakeVisible(waveform); addAndMakeVisible(mode); addAndMakeVisible(presets);
    const auto label=[this](juce::Label& l,const char* text)
    {
        l.setText(text,juce::dontSendNotification); l.setColour(juce::Label::textColourId,muted);
        l.setFont(juce::FontOptions(10.0f,juce::Font::bold)); addAndMakeVisible(l);
    };
    label(waveLabel,"WAVEFORM"); label(modeLabel,"VOICE MODE"); label(presetLabel,"FACTORY PRESET");
    keyboard.setAvailableRange(24,84); keyboard.setLowestVisibleKey(36);
    keyboard.setKeyWidth(24); keyboard.setWantsKeyboardFocus(false);
    keyboard.setColour(juce::MidiKeyboardComponent::whiteNoteColourId,juce::Colour{0xffc4ced5});
    keyboard.setColour(juce::MidiKeyboardComponent::blackNoteColourId,juce::Colour{0xff17212b});
    keyboard.setColour(juce::MidiKeyboardComponent::keyDownOverlayColourId,accent.withAlpha(.7f));
    addAndMakeVisible(keyboard);
    panic.onClick=[this] { processor.reset(); processor.keyboard.allNotesOff(0); };
    addAndMakeVisible(panic);
    startTimerHz(24);
}
RaBASSEditor::~RaBASSEditor() { stopTimer(); setLookAndFeel(nullptr); }
void RaBASSEditor::timerCallback()
{
    displayedPeak=juce::jmax(processor.peak.load(),displayedPeak*.85f);
    presets.setSelectedId(processor.getCurrentProgram()+1,juce::dontSendNotification);
    repaint(823,45,150,40);
}
void RaBASSEditor::paint(juce::Graphics& g)
{
    g.fillAll(background);
    g.setColour(accent); g.fillRoundedRectangle(24,28,5,48,2);
    g.setColour(ink); g.setFont(juce::FontOptions(46.0f,juce::Font::bold)); g.drawText("RaBASS",40,22,250,58,juce::Justification::centredLeft);
    g.setColour(muted); g.setFont(juce::FontOptions(11.0f)); g.drawText("ANALOG BASS INSTRUMENT  /  v" JucePlugin_VersionString,42,82,360,18,juce::Justification::centredLeft);
    const auto drawPanel=[&g](int x,int y,int w,const char* title)
    {
        g.setColour(panel); g.fillRoundedRectangle(static_cast<float>(x),static_cast<float>(y),static_cast<float>(w),160,10);
        g.setColour(muted); g.setFont(juce::FontOptions(11.0f,juce::Font::bold)); g.drawText(title,x+18,y+10,w-36,20,juce::Justification::centredLeft);
    };
    drawPanel(24,172,400,"01   OSCILLATORS"); drawPanel(436,172,540,"02   LOW-PASS FILTER");
    drawPanel(24,344,540,"03   AMPLITUDE"); drawPanel(576,344,400,"04   CHARACTER & OUTPUT");
    g.setColour(muted); g.setFont(juce::FontOptions(10.0f));
    g.drawText("OUTPUT",824,26,100,16,juce::Justification::centredLeft);
    g.setColour(juce::Colour{0xff26323e}); g.fillRoundedRectangle(824,52,150,7,3);
    g.setColour(displayedPeak>.95f ? amber : accent);
    const auto meter=juce::jlimit(0.0f,1.0f,(juce::Decibels::gainToDecibels(displayedPeak,-60.0f)+60)/60);
    g.fillRoundedRectangle(824,52,150*meter,7,3);
    g.setColour(muted); g.drawText("MIDI IN  /  PITCH BEND +/- 2 ST",26,516,400,20,juce::Justification::centredLeft);
    g.drawText("Independent Audio",764,612,210,18,juce::Justification::centredRight);
}
void RaBASSEditor::resized()
{
    waveLabel.setBounds(24,114,150,16); waveform.setBounds(24,134,180,28);
    modeLabel.setBounds(224,114,180,16); mode.setBounds(224,134,200,28);
    presetLabel.setBounds(676,105,280,16); presets.setBounds(676,127,300,35);
    const auto row=[this](int first,int count,int x,int y,int width)
    {
        const auto cell=width/count;
        for (int i=0;i<count;++i)
        { auto& d=dials[static_cast<size_t>(first+i)]; d.slider.setBounds(x+i*cell,y,cell,106); d.label.setBounds(x+i*cell,y+106,cell,18); }
    };
    row(0,3,30,205,388); row(3,4,442,205,528);
    row(7,4,30,377,528); row(11,3,582,377,388);
    panic.setBounds(895,514,80,24); keyboard.setBounds(24,546,952,60);
}
