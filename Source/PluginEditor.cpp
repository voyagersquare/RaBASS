#include "PluginEditor.h"
#include "RaBASSArtwork.h"
namespace
{
const juce::Colour ink{0xfff9eef0}, muted{0xffdcb9c0}, accent{0xfff2b6a4};
const juce::Colour well{0xff19070c}, edge{0xff28060e}, amber{0xfff1c88a};
const std::array<const char*,14> labels {"BLEND","DETUNE","SUB","CUTOFF","RESONANCE","ENV AMOUNT","ENV DECAY","ATTACK","DECAY","SUSTAIN","RELEASE","DRIVE","GLIDE","VOLUME"};
constexpr int editorWidth=1140, editorHeight=760;

juce::Image createMetalSurface()
{
    juce::Image result(juce::Image::RGB,editorWidth,editorHeight,false);
    juce::Image::BitmapData pixels(result,juce::Image::BitmapData::writeOnly);
    juce::Random random(0x524142415353);
    for (int y=0;y<editorHeight;++y)
    {
        const float grain=(random.nextFloat()-.5f)*9.0f;
        for (int x=0;x<editorWidth;++x)
        {
            const float u=static_cast<float>(x)/editorWidth, v=static_cast<float>(y)/editorHeight;
            const float light=11.0f*std::sin(u*juce::MathConstants<float>::pi)*std::sin(v*juce::MathConstants<float>::pi);
            const float noise=grain+(random.nextFloat()-.5f)*2.0f;
            const auto channel=[](float c) { return static_cast<juce::uint8>(juce::jlimit(0,255,juce::roundToInt(c))); };
            pixels.setPixelColour(x,y,juce::Colour(channel(48+u*65+light+noise),channel(8+u*13+light*.35f+noise*.5f),channel(15+u*24+light*.55f+noise*.65f)));
        }
    }
    return result;
}
void drawPanel(juce::Graphics& g,juce::Rectangle<float> bounds,const juce::String& title)
{
    g.setColour(juce::Colours::black.withAlpha(.27f)); g.fillRoundedRectangle(bounds.translated(1,3),3);
    g.setGradientFill(juce::ColourGradient(juce::Colour{0xff6d1c2a}.withAlpha(.76f),bounds.getTopLeft(),
        juce::Colour{0xff390b17}.withAlpha(.58f),bounds.getBottomRight(),false));
    g.fillRoundedRectangle(bounds,3);
    g.setColour(edge); g.drawRoundedRectangle(bounds,3,1.3f);
    g.setColour(juce::Colours::white.withAlpha(.14f)); g.drawRoundedRectangle(bounds.reduced(1.5f),2,1);
    const float tabWidth=juce::jmin(bounds.getWidth()-24.0f,static_cast<float>(title.length())*7.4f+24.0f);
    g.setColour(juce::Colour{0xff4a101c}); g.fillRect(bounds.getX()+12,bounds.getY()+1,tabWidth,22.0f);
    g.setColour(ink); g.setFont(juce::FontOptions(11.0f,juce::Font::bold));
    g.drawText(title,juce::Rectangle<float>(bounds.getX()+23,bounds.getY()+2,tabWidth-15,19),juce::Justification::centredLeft);
}
void drawScrew(juce::Graphics& g,float x,float y,float rotation)
{
    g.setColour(juce::Colours::black.withAlpha(.65f)); g.fillEllipse(x-12,y-10,25,25);
    g.setGradientFill(juce::ColourGradient(juce::Colour{0xffc0c7cb},x-7,y-10,juce::Colour{0xff353c42},x+8,y+10,false));
    g.fillEllipse(x-10,y-10,20,20);
    g.setColour(juce::Colour{0xff101b22}); g.drawEllipse(x-10,y-10,20,20,1.5f);
    juce::Path cross; cross.addRectangle(-6,-1.6f,12,3.2f); cross.addRectangle(-1.6f,-6,3.2f,12);
    cross.applyTransform(juce::AffineTransform::rotation(rotation).translated(x,y));
    g.setColour(juce::Colour{0xff27333d}); g.fillPath(cross);
}
juce::ColourGradient chromeGradient(juce::Rectangle<float> bounds,bool across=false)
{
    const juce::Point<float> from {across ? bounds.getX() : bounds.getCentreX(),across ? bounds.getCentreY() : bounds.getY()};
    const juce::Point<float> to {across ? bounds.getRight() : bounds.getCentreX(),across ? bounds.getCentreY() : bounds.getBottom()};
    juce::ColourGradient finish(juce::Colour{0xfff7fdff},from,juce::Colour{0xffc7d9e6},to,false);
    finish.addColour(.12,juce::Colour{0xffa4b7c6});
    finish.addColour(.28,juce::Colour{0xff344a5b});
    finish.addColour(.43,juce::Colour{0xffd3e1e9});
    finish.addColour(.49,juce::Colour{0xffffffff});
    finish.addColour(.54,juce::Colour{0xff647c8f});
    finish.addColour(.70,juce::Colour{0xff152737});
    finish.addColour(.86,juce::Colour{0xff9fb5c5});
    return finish;
}
void configureReadout(juce::Slider& slider)
{
    slider.setColour(juce::Slider::textBoxTextColourId,accent);
    slider.setColour(juce::Slider::textBoxBackgroundColourId,well);
    slider.setColour(juce::Slider::textBoxOutlineColourId,juce::Colour{0xff87424f});
}
}

BassLookAndFeel::BassLookAndFeel()
{
    setColour(juce::ComboBox::backgroundColourId,well);
    setColour(juce::ComboBox::textColourId,ink);
    setColour(juce::ComboBox::outlineColourId,edge);
    setColour(juce::PopupMenu::backgroundColourId,juce::Colour{0xff37101a});
    setColour(juce::PopupMenu::textColourId,ink);
    setColour(juce::PopupMenu::highlightedBackgroundColourId,juce::Colour{0xff7d2b3c});
    setColour(juce::TextButton::buttonColourId,juce::Colour{0xff4b1422});
    setColour(juce::TextButton::textColourOffId,well);
    setColour(juce::TextButton::textColourOnId,well);
    setColour(juce::AlertWindow::backgroundColourId,well);
    setColour(juce::AlertWindow::textColourId,ink);
    setColour(juce::TextEditor::backgroundColourId,well);
    setColour(juce::TextEditor::textColourId,ink);
    setColour(juce::TextEditor::outlineColourId,accent);
}
juce::Label* BassLookAndFeel::createSliderTextBox(juce::Slider& slider)
{
    auto* box=LookAndFeel_V4::createSliderTextBox(slider);
    box->setFont(juce::FontOptions(12.0f));
    box->setJustificationType(juce::Justification::centred);
    return box;
}
void BassLookAndFeel::drawRotarySlider(juce::Graphics& g,int x,int y,int width,int height,float value,float start,float end,juce::Slider&)
{
    const auto area=juce::Rectangle<float>(static_cast<float>(x),static_cast<float>(y),static_cast<float>(width),static_cast<float>(height));
    const auto centre=area.getCentre();
    const float scaleRadius=juce::jmin(area.getWidth(),area.getHeight())*.5f-6;
    const float radius=scaleRadius*.69f;
    constexpr float lift=10.0f;
    const auto faceCentre=centre.translated(0,-lift*.5f);
    const auto face=juce::Rectangle<float>(faceCentre.x-radius,faceCentre.y-radius,radius*2,radius*2);
    const auto base=face.translated(0,lift);
    for (int tick=0;tick<=20;++tick)
    {
        const float angle=start+static_cast<float>(tick)/20*(end-start);
        const float r1=scaleRadius-3, r2=scaleRadius+(tick%5==0 ? 2.0f : 0.0f);
        g.setColour(ink.withAlpha(tick%5==0 ? .78f : .38f));
        g.drawLine(centre.x+std::sin(angle)*r1,centre.y-std::cos(angle)*r1,
                   centre.x+std::sin(angle)*r2,centre.y-std::cos(angle)*r2,1);
    }
    juce::Path body;
    body.addEllipse(face);
    body.addRectangle(face.getX(),faceCentre.y,face.getWidth(),lift);
    body.addEllipse(base);
    juce::DropShadow(juce::Colours::black.withAlpha(.88f),6,{7,8}).drawForPath(g,body);
    g.setColour(juce::Colour{0xff06131d}); g.fillEllipse(base.expanded(2));
    g.setColour(juce::Colour{0xff9cb6c7}.withAlpha(.55f)); g.drawEllipse(base.expanded(2),.8f);
    // The polished sidewall makes the narrower control read as a taller cylinder.
    g.setGradientFill(chromeGradient(body.getBounds(),true)); g.fillPath(body);
    g.setColour(juce::Colour{0xff0b1a25}); g.drawEllipse(base,1.3f);
    g.setGradientFill(juce::ColourGradient(juce::Colours::white,face.getTopLeft(),juce::Colour{0xff172a39},face.getBottomRight(),false));
    g.fillEllipse(face);
    g.setGradientFill(chromeGradient(face.reduced(2))); g.fillEllipse(face.reduced(2));
    g.setGradientFill(juce::ColourGradient(juce::Colours::white.withAlpha(.38f),face.getX()+radius*.5f,face.getY()+radius*.4f,
        juce::Colours::transparentWhite,face.getRight(),face.getBottom(),true));
    g.fillEllipse(face.reduced(3));
    g.setColour(juce::Colour{0xff091722}.withAlpha(.7f)); g.drawEllipse(face.reduced(2),.7f);
    juce::Path crown;
    crown.addCentredArc(faceCentre.x,faceCentre.y,radius-3,radius-3,0,-1.1f,.55f,true);
    g.setColour(juce::Colours::white.withAlpha(.93f)); g.strokePath(crown,juce::PathStrokeType(1.3f));
    const float angle=start+value*(end-start);
    const auto pointer=[&](float offset,juce::Colour colour,float thickness)
    {
        g.setColour(colour);
        g.drawLine(faceCentre.x+std::sin(angle)*radius*.12f+offset,faceCentre.y-std::cos(angle)*radius*.12f+offset,
                   faceCentre.x+std::sin(angle)*radius*.79f+offset,faceCentre.y-std::cos(angle)*radius*.79f+offset,thickness);
    };
    pointer(.9f,juce::Colours::black.withAlpha(.8f),3.9f);
    pointer(0,juce::Colour{0xff071622},2.8f);
    pointer(-.65f,juce::Colours::white.withAlpha(.96f),1.0f);
}
void BassLookAndFeel::drawLinearSlider(juce::Graphics& g,int x,int y,int width,int height,float position,float,float,juce::Slider::SliderStyle style,juce::Slider&)
{
    const bool vertical=style==juce::Slider::LinearVertical;
    const float cx=static_cast<float>(x)+static_cast<float>(width)*.5f, cy=static_cast<float>(y)+static_cast<float>(height)*.5f;
    const float first=static_cast<float>(vertical ? y : x)+12, last=static_cast<float>(vertical ? y+height : x+width)-12;
    const float length=juce::jmax(1.0f,last-first);
    for (int tick=0;tick<=20;++tick)
    {
        const float p=first+length*static_cast<float>(tick)/20, mark=tick%5==0 ? 12.0f : 7.0f;
        g.setColour(ink.withAlpha(tick%5==0 ? .82f : .45f));
        if (vertical)
        { g.drawLine(cx-11-mark,p,cx-11,p,1); g.drawLine(cx+11,p,cx+11+mark,p,1); }
        else
        { g.drawLine(p,cy-11-mark,p,cy-11,1); g.drawLine(p,cy+11,p,cy+11+mark,1); }
    }
    const auto rail=vertical ? juce::Rectangle<float>(cx-4,first,8,length) : juce::Rectangle<float>(first,cy-4,length,8);
    g.setColour(juce::Colour{0xff07141c}); g.fillRoundedRectangle(rail,3);
    g.setColour(juce::Colours::white.withAlpha(.36f)); g.drawRoundedRectangle(rail.expanded(.5f),3,1);
    const float p=juce::jlimit(first,last,position);
    const auto thumb=vertical ? juce::Rectangle<float>(cx-17,p-10,34,20) : juce::Rectangle<float>(p-10,cy-17,20,34);
    juce::Path shadow; shadow.addRoundedRectangle(thumb.translated(0,3),3);
    juce::DropShadow(juce::Colours::black.withAlpha(.92f),3,{4,5}).drawForPath(g,shadow);
    g.setGradientFill(chromeGradient(thumb.translated(0,3),true)); g.fillRoundedRectangle(thumb.translated(0,3),3);
    g.setGradientFill(chromeGradient(thumb)); g.fillRoundedRectangle(thumb,3);
    g.setColour(juce::Colour{0xff0a1a27}); g.drawRoundedRectangle(thumb,3,1.2f);
    g.setColour(juce::Colours::white.withAlpha(.9f)); g.drawRoundedRectangle(thumb.reduced(1.5f),2,.7f);
    g.setColour(juce::Colour{0xff061520});
    if (vertical) g.drawLine(thumb.getX()+5,p,thumb.getRight()-5,p,2);
    else g.drawLine(p,thumb.getY()+5,p,thumb.getBottom()-5,2);
    g.setColour(ink);
    if (vertical) g.drawLine(thumb.getX()+5,p-1,thumb.getRight()-5,p-1,.7f);
    else g.drawLine(p-1,thumb.getY()+5,p-1,thumb.getBottom()-5,.7f);
}
void BassLookAndFeel::drawComboBox(juce::Graphics& g,int width,int height,bool,int,int,int,int,juce::ComboBox&)
{
    const auto bounds=juce::Rectangle<float>(0,0,static_cast<float>(width),static_cast<float>(height)).reduced(1);
    g.setColour(well); g.fillRoundedRectangle(bounds,2);
    g.setColour(edge); g.drawRoundedRectangle(bounds,2,1.5f);
    g.setColour(juce::Colour{0xff985560}); g.drawLine(2,static_cast<float>(height)-1,static_cast<float>(width)-2,static_cast<float>(height)-1,1);
    juce::Path arrow;
    const float right=static_cast<float>(width)-16, middle=static_cast<float>(height)*.5f;
    arrow.addTriangle(right-4,middle-2,right+4,middle-2,right,middle+3);
    g.setColour(accent); g.fillPath(arrow);
}
void BassLookAndFeel::drawButtonBackground(juce::Graphics& g,juce::Button& button,const juce::Colour&,bool hover,bool down)
{
    auto face=button.getLocalBounds().toFloat().reduced(4);
    face.removeFromBottom(4);
    if (down) face=face.translated(0,2);
    const auto base=face.translated(0,down ? 2.0f : 4.0f);
    juce::Path shadow; shadow.addRoundedRectangle(base,3);
    juce::DropShadow(juce::Colours::black.withAlpha(down ? .6f : .9f),2,{2,3}).drawForPath(g,shadow);
    g.setGradientFill(chromeGradient(base,true)); g.fillRoundedRectangle(base,3);
    auto finish=chromeGradient(face);
    finish.addColour(.28,juce::Colour{0xff8fa3b1});
    finish.addColour(.54,juce::Colour{0xffadbfcc});
    finish.addColour(.70,juce::Colour{0xff7b91a2});
    if (hover) finish.addColour(.4,juce::Colour{0xffe8faff});
    g.setGradientFill(finish); g.fillRoundedRectangle(face,3);
    g.setColour(juce::Colour{0xff071724}); g.drawRoundedRectangle(face,3,1);
    g.setColour(juce::Colours::white.withAlpha(.92f)); g.drawRoundedRectangle(face.reduced(1),2,.8f);
}
void BassLookAndFeel::drawButtonText(juce::Graphics& g,juce::TextButton& button,bool,bool down)
{
    g.setFont(juce::FontOptions(button.getWidth()>60 ? 12.0f : 14.0f,juce::Font::bold));
    auto textBounds=button.getLocalBounds().reduced(6,5).translated(0,down ? 0 : -2);
    // A dark face legend stays legible over the bright chrome reflections.
    g.setColour(juce::Colours::white.withAlpha(.85f));
    g.drawFittedText(button.getButtonText(),textBounds.translated(0,1),juce::Justification::centred,1,.8f);
    g.setColour(juce::Colour{0xff061521});
    g.drawFittedText(button.getButtonText(),textBounds,juce::Justification::centred,1,.8f);
}
void BassSlider::mouseWheelMove(const juce::MouseEvent& event,const juce::MouseWheelDetails& wheel)
{
    auto adjusted=wheel;
    if (fineCutoff)
    {
        const float scale=event.mods.isShiftDown() ? .01f : .1f;
        adjusted.deltaX*=scale; adjusted.deltaY*=scale;
    }
    // Keep JUCE's wheel direction, trackpad handling and host automation gestures.
    juce::Slider::mouseWheelMove(event,adjusted);
}
bool BassSlider::keyPressed(const juce::KeyPress& key)
{
    const int code=key.getKeyCode();
    if (fineCutoff && isEnabled() && !key.getModifiers().isCtrlDown()
        && !key.getModifiers().isAltDown() && !key.getModifiers().isCommandDown()
        && (code==juce::KeyPress::upKey || code==juce::KeyPress::rightKey
            || code==juce::KeyPress::downKey || code==juce::KeyPress::leftKey))
    {
        const double direction=(code==juce::KeyPress::upKey || code==juce::KeyPress::rightKey) ? 1.0 : -1.0;
        const juce::Slider::ScopedDragNotification gesture(*this);
        setValue(getValue()+direction*(key.getModifiers().isShiftDown() ? .1 : 1.0),juce::sendNotificationSync);
        return true;
    }
    return juce::Slider::keyPressed(key);
}
RaBASSEditor::RaBASSEditor(RaBASSProcessor& p)
    : AudioProcessorEditor(p), processor(p), keyboard(p.keyboard,juce::MidiKeyboardComponent::horizontalKeyboard),
      metalSurface(createMetalSurface()),
      wordmark(juce::ImageCache::getFromMemory(RaBASSArtwork::RaBassLogo_png,RaBASSArtwork::RaBassLogo_pngSize))
{
    setLookAndFeel(&look); setSize(editorWidth,editorHeight); setResizable(false,false);
    const auto setLabel=[this](juce::Label& label,const char* text)
    {
        label.setText(text,juce::dontSendNotification);
        label.setFont(juce::FontOptions(11.0f,juce::Font::bold));
        label.setColour(juce::Label::textColourId,ink);
        label.setJustificationType(juce::Justification::centred);
        addAndMakeVisible(label);
    };
    for (size_t i=0;i<dials.size();++i)
    {
        auto& dial=dials[i]; const juce::String id=parameterIds[i+2];
        dial.slider.setName(labels[i]);
        dial.slider.setSliderStyle(i==12 ? juce::Slider::LinearVertical : juce::Slider::RotaryHorizontalVerticalDrag);
        dial.slider.setTextBoxStyle(juce::Slider::TextBoxBelow,false,86,20);
        configureReadout(dial.slider);
        dial.slider.setPopupDisplayEnabled(false,false,nullptr);
        dial.slider.setTooltip(processor.parameters.getParameter(id)->getName(64)+". Double-click to reset.");
        if (id=="cutoff")
        {
            dial.slider.enableFineCutoff();
            dial.slider.setTooltip("Filter cutoff. Drag for gradual adjustment; Shift + mouse wheel for extra precision. Arrow keys: 1 Hz; Shift + arrow: 0.1 Hz. Type Hz or kHz in the value box. Double-click to reset.");
        }
        dial.attachment=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.parameters,id,dial.slider);
        dial.slider.textFromValueFunction=[id](double v)
        {
            if (id=="cutoff") return juce::String(v,1)+" Hz";
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
        dial.slider.updateText(); addAndMakeVisible(dial.slider); setLabel(dial.label,labels[i]);
    }
    const std::array<const char*,3> oscillatorNames {"SAWTOOTH", "TRIANGLE", "SINE"};
    for (size_t i=0;i<oscillatorLevels.size();++i)
    {
        auto& level=oscillatorLevels[i]; level.slider.setName(oscillatorNames[i]);
        level.slider.setSliderStyle(juce::Slider::LinearVertical);
        level.slider.setTextBoxStyle(juce::Slider::TextBoxBelow,false,72,20);
        configureReadout(level.slider);
        level.attachment=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.parameters,parameterIds[16+i],level.slider);
        level.slider.textFromValueFunction=[](double value) { return juce::String(value*100,0)+" %"; };
        level.slider.valueFromTextFunction=[](const juce::String& text) { return text.getDoubleValue()/100; };
        level.slider.updateText(); addAndMakeVisible(level.slider); setLabel(level.label,oscillatorNames[i]);
    }
    mode.setName("VOICE MODE"); presets.setName("PRESETS");
    mode.addItemList({"Mono / legato","Poly / 8 voices"},1);
    presets.beforePopup=[this] { refreshPresetMenu(); };
    presets.onChange=[this]
    {
        const int id=presets.getSelectedId();
        if (id>=1 && id<=processor.getNumPrograms()) processor.setCurrentProgram(id-1);
        else if (juce::isPositiveAndBelow(id-1001,static_cast<int>(userPresets.size())))
        {
            const auto result=processor.loadUserPreset(userPresets[static_cast<size_t>(id-1001)].file);
            if (result.failed()) { showPresetError(result.getErrorMessage()); refreshPresetMenu(); }
        }
    };
    refreshPresetMenu();
    modeAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(p.parameters,"mode",mode);
    addAndMakeVisible(mode); addAndMakeVisible(presets); setLabel(modeLabel,"VOICE MODE");
    keyboard.setName("MIDI KEYBOARD");
    keyboard.setAvailableRange(24,84); keyboard.setLowestVisibleKey(36); keyboard.setKeyWidth(25); keyboard.setWantsKeyboardFocus(false);
    keyboard.setColour(juce::MidiKeyboardComponent::whiteNoteColourId,juce::Colour{0xffd5dfe3});
    keyboard.setColour(juce::MidiKeyboardComponent::blackNoteColourId,juce::Colour{0xff200c13});
    keyboard.setColour(juce::MidiKeyboardComponent::keyDownOverlayColourId,accent.withAlpha(.65f)); addAndMakeVisible(keyboard);
    panic.setName("PANIC"); previousPreset.setName("PREVIOUS PRESET"); nextPreset.setName("NEXT PRESET");
    panic.onClick=[this] { processor.reset(); processor.keyboard.allNotesOff(0); };
    previousPreset.onClick=[this] { navigatePreset(-1); };
    nextPreset.onClick=[this] { navigatePreset(1); };
    savePreset.setName("SAVE PRESET"); savePreset.onClick=[this] { beginSavePreset(); };
    for (auto* button : {&panic,&previousPreset,&nextPreset,&savePreset})
    { button->setWantsKeyboardFocus(false); addAndMakeVisible(button); }
    startTimerHz(24);
}
RaBASSEditor::~RaBASSEditor()
{
    stopTimer();
    if (presetDialog!=nullptr) { presetDialog->setLookAndFeel(nullptr); presetDialog->exitModalState(0); }
    setLookAndFeel(nullptr);
}
void RaBASSEditor::timerCallback()
{
    displayedPeak=juce::jmax(processor.peak.exchange(0,std::memory_order_relaxed),displayedPeak*.85f);
    syncPresetSelection(); repaint(972,504,128,46);
}
void RaBASSEditor::paint(juce::Graphics& g)
{
    g.drawImageAt(metalSurface,0,0);
    g.setColour(juce::Colours::black.withAlpha(.34f)); g.drawRect(getLocalBounds(),2);
    g.setColour(juce::Colours::white.withAlpha(.17f)); g.drawRect(getLocalBounds().reduced(3),1);
    g.setColour(juce::Colours::black.withAlpha(.35f)); g.fillRect(42,12,1056,70);
    g.setColour(juce::Colours::black.withAlpha(.6f));
    g.drawImageWithin(wordmark,53,16,190,64,juce::RectanglePlacement::centred,true);
    g.setColour(ink);
    g.drawImageWithin(wordmark,52,14,190,64,juce::RectanglePlacement::centred,true);
    g.setFont(juce::FontOptions(11.0f,juce::Font::bold)); g.setColour(muted);
    g.drawText("SUBTRACTIVE BASS SYNTH",248,25,330,18,juce::Justification::centredLeft);
    g.setColour(amber); g.setFont(juce::FontOptions(10.0f));
    g.drawText("UI PREVIEW 06  /  v" JucePlugin_VersionString,248,49,330,18,juce::Justification::centredLeft);
    drawPanel(g,{28,104,636,234},"BASS GENERATOR");
    drawPanel(g,{680,104,260,454},"FILTER MODULE");
    drawPanel(g,{956,104,156,454},"OUTPUT MODULE");
    drawPanel(g,{28,354,636,204},"AMPLITUDE & GLIDE");
    drawPanel(g,{28,574,1084,66},"PRESET MANAGER");
    g.setFont(juce::FontOptions(10.0f)); g.setColour(muted);
    g.drawText("OSCILLATOR VOICING",340,142,300,18,juce::Justification::centredLeft);
    g.drawText("Mix the three waveforms",45,316,268,16,juce::Justification::centred);
    g.drawText("RESONANT LOW-PASS",696,143,228,18,juce::Justification::centred);
    g.drawText("FILTER ENVELOPE",696,323,228,18,juce::Justification::centred);
    g.drawText("12 dB / OCT",696,516,228,18,juce::Justification::centred);
    g.drawText("ENVELOPE SHAPING",45,380,580,18,juce::Justification::centredLeft);
    g.drawText("MIDI IN  /  PITCH BEND +/- 2 ST",700,603,382,24,juce::Justification::centredRight);
    g.setColour(ink.withAlpha(.12f)); g.drawLine(328,145,328,316,1); g.drawLine(698,313,922,313,1);
    const auto meter=juce::jlimit(0.0f,1.0f,(juce::Decibels::gainToDecibels(displayedPeak,-60.0f)+60)/60);
    for (int segment=0;segment<14;++segment)
    {
        g.setColour(static_cast<float>(segment)/14<meter ? (segment>11 ? amber : accent) : well);
        g.fillRect(978+segment*8,516,6,9);
    }
    g.setColour(muted); g.setFont(juce::FontOptions(9.0f));
    g.drawText("OUTPUT LEVEL",978,532,112,14,juce::Justification::centred);
    g.drawText("INDEPENDENT AUDIO",52,735,330,16,juce::Justification::centredLeft);
    g.drawText("RaBASS  /  UI PREVIEW 06",780,735,308,16,juce::Justification::centredRight);
    drawScrew(g,22,24,.5f); drawScrew(g,1118,24,-.4f); drawScrew(g,22,736,-.35f); drawScrew(g,1118,736,.6f);
}
void RaBASSEditor::resized()
{
    const auto dial=[this](size_t index,int x,int y,int width,int height)
    {
        dials[index].slider.setBounds(x,y,width,height);
        dials[index].label.setBounds(x,y+height+4,width,18);
    };
    for (size_t i=0;i<oscillatorLevels.size();++i)
    {
        const int x=48+static_cast<int>(i)*92;
        oscillatorLevels[i].label.setBounds(x,138,80,18);
        oscillatorLevels[i].slider.setBounds(x,160,80,150);
    }
    dial(0,340,178,98,112); dial(1,446,178,98,112); dial(2,552,178,98,112);
    dial(3,696,168,104,112); dial(4,818,168,104,112); dial(5,696,352,104,112); dial(6,818,352,104,112);
    const std::array<size_t,5> envelope {7,8,9,10,12};
    for (size_t i=0;i<envelope.size();++i) dial(envelope[i],44+static_cast<int>(i)*122,404,112,120);
    dial(13,978,154,112,126); dial(11,978,338,112,126);
    modeLabel.setBounds(736,20,190,18); mode.setBounds(736,43,190,28); panic.setBounds(978,40,104,34);
    presets.setBounds(50,602,410,28); savePreset.setBounds(470,600,104,34);
    previousPreset.setBounds(592,600,30,34); nextPreset.setBounds(636,600,30,34);
    keyboard.setBounds(52,656,1036,70);
}
