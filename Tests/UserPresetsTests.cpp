#include "PluginProcessor.h"
#include <cstdlib>
#include <iostream>

namespace
{
void require(bool ok,const char* message)
{
    if (!ok) { std::cerr << "FAIL: " << message << '\n'; std::exit(1); }
}
struct PresetFolder
{
    juce::File file=juce::File::getSpecialLocation(juce::File::tempDirectory)
        .getChildFile("RaBASS-presets-test-"+juce::Uuid().toString());
    ~PresetFolder() { file.deleteRecursively(); }
};
void setValue(RaBASSProcessor& p,const char* id,float value)
{
    auto* parameter=p.parameters.getParameter(id);
    parameter->setValueNotifyingHost(parameter->convertTo0to1(value));
}
float value(RaBASSProcessor& p,const char* id) { return p.parameters.getRawParameterValue(id)->load(); }
}
int main()
{
    juce::ScopedJuceInitialiser_GUI gui;
    PresetFolder folder;
    RaBASSProcessor p(folder.file);
    require(p.getNumPrograms()==18,"original six plus twelve new factory presets");
    require(p.getUserPresets().empty() && !folder.file.exists(),"opening an empty library does not create files");
    p.setCurrentProgram(12);
    setValue(p,"cutoff",432); setValue(p,"sawLevel",.21f); setValue(p,"triangleLevel",.63f); setValue(p,"sineLevel",.9f);
    std::array<float,19> saved{};
    for (size_t i=0;i<saved.size();++i) saved[i]=value(p,parameterIds[i]);
    require(p.saveUserPreset("Night Bass").wasOk(),"save user preset");
    require(p.getCurrentUserPresetName()=="Night Bass","saved preset becomes the selected user preset");
    auto list=p.getUserPresets();
    require(list.size()==1 && list[0].name=="Night Bass","saved preset appears in the user library");
    require(list[0].file.getParentDirectory()==folder.file,"preset stays inside its dedicated directory");
    const auto originalBytes=list[0].file.loadFileAsString();
    setValue(p,"cutoff",1234);
    require(p.saveUserPreset("night bass").failed(),"case-insensitive duplicate requires explicit replacement");
    require(list[0].file.loadFileAsString()==originalBytes,"declining replacement preserves the file");
    require(p.loadUserPreset(list[0].file).wasOk(),"reload saved sound");
    for (size_t i=0;i<saved.size();++i)
        require(std::abs(value(p,parameterIds[i])-saved[i])<.001f,"all parameters return to the saved values");

    RaBASSProcessor restarted(folder.file);
    require(restarted.getUserPresets().size()==1,"user library persists across new plugin instances");
    require(restarted.loadUserPreset(list[0].file).wasOk() && std::abs(value(restarted,"cutoff")-432)<.01f,"new instance restores the saved sound");
    juce::MemoryBlock project;
    restarted.getStateInformation(project);
    PresetFolder otherFolder;
    RaBASSProcessor restoredProject(otherFolder.file);
    restoredProject.setStateInformation(project.getData(),static_cast<int>(project.getSize()));
    require(restoredProject.getCurrentUserPresetName()=="Night Bass","DAW project restores the user preset name without its file");
    require(std::abs(value(restoredProject,"cutoff")-432)<.01f,"DAW project retains the sound independently of the preset library");
    restoredProject.setCurrentProgram(0);
    require(restoredProject.getCurrentUserPresetName().isEmpty(),"factory preset selection clears the user preset label");

    setValue(p,"cutoff",987);
    require(p.saveUserPreset("Night Bass",true).wasOk(),"explicit replacement saves the new sound");
    require(p.getUserPresets().size()==1,"replacement does not create duplicates");
    require(restarted.loadUserPreset(list[0].file).wasOk() && std::abs(value(restarted,"cutoff")-987)<.01f,"replacement can be loaded by another instance");
    require(p.saveUserPreset("   ").failed() && p.saveUserPreset("../outside").failed()
        && p.saveUserPreset(juce::String::repeatedString("x",65)).failed(),"invalid preset names are rejected");

    const auto corrupt=folder.file.getChildFile("corrupt.rabasspreset");
    require(corrupt.replaceWithText("not a preset"),"create invalid-file fixture");
    require(p.getUserPresets().size()==1,"corrupt files are excluded from the dropdown library");
    const auto before=value(p,"cutoff");
    require(p.loadUserPreset(corrupt).failed() && std::abs(value(p,"cutoff")-before)<.01f,"invalid files do not change the current sound");
    auto malformed=juce::XmlDocument::parse(list[0].file.loadFileAsString());
    malformed->getFirstChildElement()->getChildByAttribute("id","cutoff")->setAttribute("value","nan");
    require(malformed->writeTo(corrupt),"write invalid-parameter fixture");
    require(p.loadUserPreset(corrupt).failed() && p.getUserPresets().size()==1,"non-finite parameter values are rejected");

    // Check the actual menu structure, saved-user selection and Save button layout.
    std::unique_ptr<juce::AudioProcessorEditor> editor(p.createEditor());
    juce::ComboBox* menu=nullptr;
    juce::TextButton* saveButton=nullptr;
    for (auto* child : editor->getChildren())
    {
        if (child->getName()=="PRESETS") menu=dynamic_cast<juce::ComboBox*>(child);
        if (child->getName()=="SAVE PRESET") saveButton=dynamic_cast<juce::TextButton*>(child);
    }
    require(menu && saveButton,"preset menu and Save Preset control exist");
    bool factoryHeading=false,userHeading=false,userItem=false;
    for (juce::PopupMenu::MenuItemIterator item(*menu->getRootMenu()); item.next();)
    {
        factoryHeading|=item.getItem().isSectionHeader && item.getItem().text=="Factory presets";
        userHeading|=item.getItem().isSectionHeader && item.getItem().text=="User presets";
        userItem|=item.getItem().text=="Night Bass" && item.getItem().itemID>=1001;
    }
    require(factoryHeading && userHeading && userItem,"dropdown contains factory and user sections with saved sounds");
    require(menu->getSelectedId()>=1001,"saved user preset is selected in the editor");
    require(!menu->getBounds().intersects(saveButton->getBounds()),"Save button does not overlap the dropdown");
   #if JUCE_WINDOWS
    // Exercise the real dialog on the supported plugin platform; portable
    // storage/menu tests above also run without a Linux window manager.
    saveButton->onClick();
    auto* dialog=dynamic_cast<juce::AlertWindow*>(juce::ModalComponentManager::getInstance()->getModalComponent(0));
    require(dialog && dialog->getTextEditor("name"),"Save Preset opens the name-entry dialog");
    dialog->getTextEditor("name")->setText("From UI",false);
    dialog->exitModalState(1);
    juce::Timer::callAfterDelay(100,[] { juce::MessageManager::getInstance()->stopDispatchLoop(); });
    juce::MessageManager::getInstance()->runDispatchLoop();
    require(p.userPresetExists("From UI") && p.getCurrentUserPresetName()=="From UI","Save dialog callback persists the new preset");
    require(menu->getSelectedId()>=1001 && menu->getText()=="From UI","new user preset is selected immediately");
   #endif
    std::cout << "PASS: user preset persistence, replacement, validation, project recall and preset menu\n";
}
