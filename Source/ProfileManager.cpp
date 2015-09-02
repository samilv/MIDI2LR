/*
  ==============================================================================

    ProfileManager.cpp
    Created: 29 Aug 2015 10:27:13pm
    Author:  Parth

  ==============================================================================
*/

#include "ProfileManager.h"
#include "CommandMap.h"
#include "LRCommands.h"

ProfileManager& ProfileManager::getInstance()
{
    static ProfileManager instance;
    return instance;
}

ProfileManager::ProfileManager() : _currentProfileIdx(0)
{
    MIDIProcessor::getInstance().addMIDICommandListener(this);
}

void ProfileManager::addListener(ProfileChangeListener *listener)
{
    _listeners.addIfNotAlreadyThere(listener);
}

void ProfileManager::setProfileDirectory(File& dir)
{
    _profileLocation = dir;

    Array<File> fileArray;
    dir.findChildFiles(fileArray, File::findFiles, false, "*.xml");

    _profiles.clear();
    for (auto file : fileArray)
        _profiles.add(file.getFileName());

    if (_profiles.size() > 0)
        switchToProfile(_profiles[0]);
}

StringArray& ProfileManager::getMenuItems()
{
    return _profiles;
}

void ProfileManager::switchToProfile(int profileIdx)
{
    if (profileIdx >= 0 && profileIdx < _profiles.size())
    {
        switchToProfile(_profiles[profileIdx]);
        _currentProfileIdx = profileIdx;
    }
}

void ProfileManager::switchToProfile(const String& profile)
{
    File profileFile = _profileLocation.getChildFile(profile);

    if (profileFile.exists())
    {
        ScopedPointer<XmlElement> elem = XmlDocument::parse(profileFile);
        for (auto listener : _listeners)
            listener->profileChanged(elem);
    }
}

void ProfileManager::switchToPreviousProfile()
{
    _currentProfileIdx--;
    if (_currentProfileIdx < 0) _currentProfileIdx = _profiles.size() - 1;

    switchToProfile(_profiles[_currentProfileIdx]);
}

void ProfileManager::switchToNextProfile()
{
    _currentProfileIdx++;
    if (_currentProfileIdx == _profiles.size()) _currentProfileIdx = 0;

    switchToProfile(_profiles[_currentProfileIdx]);
}

void ProfileManager::handleMidiCC(int midiChannel, int controller, int value)
{
    MIDI_Message cc(midiChannel, controller, true);

    if (value == 0 || !CommandMap::getInstance().messageExistsInMap(cc) ||
        CommandMap::getInstance().getCommandforMessage(cc) < LRCommandList::LRStringList.size())
        return;

    if (CommandMap::getInstance().getCommandforMessage(cc) - LRCommandList::LRStringList.size() == 0)
        _switchState = SWITCH_STATE::PREV;
    else if (CommandMap::getInstance().getCommandforMessage(cc) - LRCommandList::LRStringList.size() == 1)
        _switchState = SWITCH_STATE::NEXT;

    triggerAsyncUpdate();
}

void ProfileManager::handleMidiNote(int midiChannel, int note)
{
    MIDI_Message note_msg(midiChannel, note, false);

    if (!CommandMap::getInstance().messageExistsInMap(note_msg) ||
        CommandMap::getInstance().getCommandforMessage(note_msg) < LRCommandList::LRStringList.size())
        return;

    if (CommandMap::getInstance().getCommandforMessage(note_msg) - LRCommandList::LRStringList.size() == 0)
        _switchState = SWITCH_STATE::PREV;
    else if (CommandMap::getInstance().getCommandforMessage(note_msg) - LRCommandList::LRStringList.size() == 1)
        _switchState = SWITCH_STATE::NEXT;

    triggerAsyncUpdate();
}

void ProfileManager::handleAsyncUpdate()
{
    switch (_switchState)
    {
    case SWITCH_STATE::PREV:
        switchToPreviousProfile();
        _switchState = SWITCH_STATE::NONE;
        break;
    case SWITCH_STATE::NEXT:
        switchToNextProfile();
        _switchState = SWITCH_STATE::NONE;
        break;
    default:
        break;
    }
}