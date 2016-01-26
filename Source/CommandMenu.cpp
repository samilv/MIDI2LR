/*
  ==============================================================================

    CommandMenu.cpp
    Created: 3 Aug 2015 3:27:33pm
    Author:  Parth, Jaffe

This file is part of MIDI2LR. Copyright 2015-2016 by Rory Jaffe.

MIDI2LR is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

MIDI2LR is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
MIDI2LR.  If not, see <http://www.gnu.org/licenses/>.
  ==============================================================================
*/

#include <limits>
#include "CommandMenu.h"
#include "LRCommands.h"
#include "Tools.h"

CommandMenu::CommandMenu(const MIDI_Message& msg): _msg(msg),
_selectedItem(std::numeric_limits<unsigned int>::max()),
TextButton("Unmapped"), m_commandMap(nullptr)
{
    
}

void CommandMenu::setMsg(const MIDI_Message& msg)
{
    _msg = msg;
}

void CommandMenu::buttonClicked(Button* UNUSED_ARG(button))
{
    size_t idx = 1;
    bool subMenuTickSet = false;
    PopupMenu mainMenu;
    mainMenu.addItem(idx, "Unmapped", true, subMenuTickSet = (idx == _selectedItem));
    idx++;

    const std::vector<String> menus = { "Basic",
                                        "Tone Curve",
                                        "HSL / Color / B&W" ,
                                        "Reset HSL / Color / B&W" ,
                                        "Split Toning",
                                        "Detail",
                                        "Lens Correction",
                                        "Effects",
                                        "Camera Calibration",
                                        "Photo Actions",
                                        "Develop Presets",
                                        "Miscellaneous",
                                        "Go To Tool, Module, or Panel",
                                        "View Modes",
                                        "Profiles",
                                        // MIDI2LR items
                                        "Next/Prev Profile"
    };
    const std::vector<std::vector<String>> menuEntries = { LRCommandList::AdjustmentStringList,
                                                           LRCommandList::ToneStringList,
                                                           LRCommandList::MixerStringList,
                                                           LRCommandList::ResetMixerStringList,
                                                           LRCommandList::SplitToningStringList,
                                                           LRCommandList::DetailStringList,
                                                           LRCommandList::LensCorrectionStringList,
                                                           LRCommandList::EffectsStringList,
                                                           LRCommandList::CalibrateStringList,
                                                           LRCommandList::SelectionList,
                                                           LRCommandList::PresetsList,
                                                           LRCommandList::MiscList,
                                                           LRCommandList::TMPList,
                                                           LRCommandList::ViewModesList,
                                                           LRCommandList::ProfilesList,
                                                            // MIDI2LR items
                                                            LRCommandList::NextPrevProfile,
    };

    // add each submenu
    for (size_t menuIdx = 0; menuIdx < menus.size(); menuIdx++)
    {
        PopupMenu subMenu;
        for (auto cmd : menuEntries[menuIdx])
        {
            bool alreadyMapped = false;
			if ((idx - 1 < LRCommandList::LRStringList.size()) && (m_commandMap))
			{
				alreadyMapped = m_commandMap->commandHasAssociatedMessage(LRCommandList::LRStringList[idx - 1]);
			}

            // add each submenu entry, ticking the previously selected entry and disabling a previously mapped entry

            if (alreadyMapped)
                subMenu.addColouredItem(idx, cmd, Colours::red, true, idx == _selectedItem);
            else
                subMenu.addItem(idx, cmd, true, idx == _selectedItem);

            idx++;
        }
        // set whether or not the submenu is ticked (true if one of the submenu's entries is selected)
        mainMenu.addSubMenu(menus[menuIdx], subMenu, true, nullptr, _selectedItem < idx && !subMenuTickSet);
        subMenuTickSet |= (_selectedItem < idx && !subMenuTickSet);
    }

	unsigned int result = mainMenu.show();
    if ((result) && (m_commandMap))
    {
        // user chose a different command, remove previous command mapping associated to this menu
        if (_selectedItem < std::numeric_limits<unsigned int>::max())
			m_commandMap->removeMessage(_msg);

        if (result - 1 < LRCommandList::LRStringList.size())
            setButtonText(LRCommandList::LRStringList[result - 1]);
        else
            setButtonText(LRCommandList::NextPrevProfile[result - 1 - LRCommandList::LRStringList.size()]);

        _selectedItem = result;

        // Map the selected command to the CC
		m_commandMap->addCommandforMessage(result - 1, _msg);
    }
}

void CommandMenu::setSelectedItem(unsigned int idx)
{
    _selectedItem = idx;
    if (idx - 1 < LRCommandList::LRStringList.size())
        setButtonText(LRCommandList::LRStringList[idx - 1]);
    else
        setButtonText(LRCommandList::NextPrevProfile[idx - 1 - LRCommandList::LRStringList.size()]);
}

void CommandMenu::Init(CommandMap *mapCommand)
{
	//copy the pointer
	m_commandMap = mapCommand;
	addListener(this);
}