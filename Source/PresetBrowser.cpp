#include "PluginEditor.h"

void RaBASSEditor::refreshPresetMenu()
{
    userPresets=processor.getUserPresets();
    presets.clear(juce::dontSendNotification);
    presets.addSectionHeading("Factory presets");
    for (int i=0;i<processor.getNumPrograms();++i) presets.addItem(processor.getProgramName(i),i+1);
    presets.addSeparator();
    presets.addSectionHeading("User presets");
    if (userPresets.empty())
    { presets.addItem("No user presets yet",2000); presets.setItemEnabled(2000,false); }
    else
        for (size_t i=0;i<userPresets.size();++i) presets.addItem(userPresets[i].name,1001+static_cast<int>(i));
    syncPresetSelection();
}
void RaBASSEditor::syncPresetSelection()
{
    const auto name=processor.getCurrentUserPresetName();
    if (name.isEmpty()) { presets.setSelectedId(processor.getCurrentProgram()+1,juce::dontSendNotification); return; }
    for (size_t i=0;i<userPresets.size();++i)
        if (userPresets[i].name.equalsIgnoreCase(name))
        { presets.setSelectedId(1001+static_cast<int>(i),juce::dontSendNotification); return; }
    // A DAW project retains the sound even if its preset file is unavailable.
    presets.setText("User: "+name,juce::dontSendNotification);
}
void RaBASSEditor::navigatePreset(int direction)
{
    refreshPresetMenu();
    std::vector<int> ids;
    for (int i=0;i<processor.getNumPrograms();++i) ids.push_back(i+1);
    for (size_t i=0;i<userPresets.size();++i) ids.push_back(1001+static_cast<int>(i));
    int current=processor.getCurrentProgram();
    for (size_t i=0;i<ids.size();++i) if (ids[i]==presets.getSelectedId()) current=static_cast<int>(i);
    const int next=(current+direction+static_cast<int>(ids.size()))%static_cast<int>(ids.size());
    presets.setSelectedId(ids[static_cast<size_t>(next)],juce::sendNotificationSync);
}
juce::AlertWindow* RaBASSEditor::makePresetDialog(const juce::String& title,const juce::String& message)
{
    auto* window=new juce::AlertWindow(title,message,juce::MessageBoxIconType::NoIcon,this);
    window->setLookAndFeel(&look);
    presetDialog=window;
    return window;
}
void RaBASSEditor::beginSavePreset()
{
    if (presetDialog!=nullptr) { presetDialog->toFront(true); return; }
    auto* window=makePresetDialog("Save user preset","Name the current sound. It will appear under User presets.");
    const auto selected=processor.getCurrentUserPresetName();
    window->addTextEditor("name",selected.isEmpty() ? "New bass" : selected,"Preset name");
    window->getTextEditor("name")->setInputRestrictions(64);
    window->addButton("Save",1,juce::KeyPress{juce::KeyPress::returnKey});
    window->addButton("Cancel",0,juce::KeyPress{juce::KeyPress::escapeKey});
    const juce::Component::SafePointer<RaBASSEditor> self(this);
    const juce::Component::SafePointer<juce::AlertWindow> dialog(window);
    window->enterModalState(true,juce::ModalCallbackFunction::create([self,dialog](int result)
    {
        if (self==nullptr || dialog==nullptr) return;
        dialog->setLookAndFeel(nullptr);
        self->presetDialog=nullptr;
        if (result==1) self->saveNamedPreset(dialog->getTextEditorContents("name").trim(),false);
    }),true);
}
void RaBASSEditor::saveNamedPreset(const juce::String& name,bool replaceExisting)
{
    if (!replaceExisting && processor.userPresetExists(name))
    {
        auto* window=makePresetDialog("Replace user preset?","A user preset named \""+name+"\" already exists.");
        window->addButton("Replace",1,juce::KeyPress{juce::KeyPress::returnKey});
        window->addButton("Cancel",0,juce::KeyPress{juce::KeyPress::escapeKey});
        const juce::Component::SafePointer<RaBASSEditor> self(this);
        const juce::Component::SafePointer<juce::AlertWindow> dialog(window);
        window->enterModalState(true,juce::ModalCallbackFunction::create([self,dialog,name](int result)
        {
            if (self==nullptr) return;
            if (dialog!=nullptr) dialog->setLookAndFeel(nullptr);
            self->presetDialog=nullptr;
            if (result==1) self->saveNamedPreset(name,true);
        }),true);
        return;
    }
    const auto result=processor.saveUserPreset(name,replaceExisting);
    if (result.failed()) showPresetError(result.getErrorMessage());
    else refreshPresetMenu();
}
void RaBASSEditor::showPresetError(const juce::String& message)
{
    auto* window=makePresetDialog("RaBASS presets",message);
    window->addButton("OK",0,juce::KeyPress{juce::KeyPress::returnKey},juce::KeyPress{juce::KeyPress::escapeKey});
    window->enterModalState(true,nullptr,true);
}
