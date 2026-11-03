// Copyright(C) 2021 by Steven Adler
//
// This file is part of Fusion Media Player plugin for OpenCPN.
//
// Fusion Media Player plugin for OpenCPN is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Fusion Media Player plugin for OpenCPN is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with the Fusion Media Player plugin for OpenCPN. If not, see <https://www.gnu.org/licenses/>.
//
// NMEA2000® is a registered trademark of the National Marine Electronics Association
// Fusion® is a registered trademark of Fusion Entertainment
// 
// This software has been developed independently of Fusion Entertainment and is not
// sanctioned nor supported by them.
//
// Project: Fusion Media Player Plugin
// Description: Remotely Control Fusion Media Players (via TwoCan plugin)
// Owner: twocanplugin@hotmail.com
// Date: 10/10/2021
// Version History:
// 1.0 Initial Release
// 01/10/2026 - 2.0 Independant of TwoCan plugin, uses OpenCPN NMEA 2000 interfaces
//

#include "mediaplayer_dialog.h"

// Media File/Folder Client Metadata used for folders & music tracks from media devices
MediaFileItem::MediaFileItem(const int id, const MEDIA_FILE_TYPE type, const int level) {
	itemType = type;
	itemId = id;
	itemLevel = level;
}

MediaFileItem::~MediaFileItem() {
}

int MediaFileItem::GetId() {
	return itemId;
}

MEDIA_FILE_TYPE MediaFileItem::GetMediaType() {
	return itemType;
}

int MediaFileItem::GetLevel() {
	return itemLevel;
}

// The plugin dialog, inherits from a base class developed using wxFormBuilder
MediaPlayerDialog::MediaPlayerDialog( wxWindow* parent, wxEvtHandler *handler) : MediaPlayerDialogBase(parent) {
	eventHandler = handler;
	SetMinSize(wxSize(500,500));
	Fit();
	Layout();

	}

MediaPlayerDialog::~MediaPlayerDialog(void) {
}

// Event Handlers

// Clicking on an item in the media list box
// BUG BUG, When navigating back up, would be nice to highlight where we were
// BUG BUG Include the path (breadcrumbs) for the folders.
// eg. iAudio\Music\Chain\Towards The Blues\.. 
void MediaPlayerDialog::OnFileSelected(wxCommandEvent& event) {

	int itemNumber = event.GetSelection();
	wxListBox *listBox = wxDynamicCast(event.GetEventObject(), wxListBox);
	MediaFileItem *selectedItemData = NULL;

	if (listBox != NULL && itemNumber != wxNOT_FOUND) {
		selectedItemData = static_cast<MediaFileItem*>(listBox->GetClientObject(itemNumber));

		int fileId = selectedItemData->GetId();
		int level = selectedItemData->GetLevel();
		MEDIA_FILE_TYPE mediaType = selectedItemData->GetMediaType();

		// if it is a radio station, change to that frequency
		if (mediaType == MEDIA_FILE_TYPE::RADIO) {
			wxCommandEvent* commandEvent = new wxCommandEvent(wxEVT_MEDIAPLAYER_DIALOG_EVENT, MEDIAPLAYER_SELECT_PRESET);
			// Note to self, for radio station presets, the fileId contains the 
			// frequency and level contains the index into the list if presets
			commandEvent->SetInt(fileId);
			wxQueueEvent(eventHandler, commandEvent);
			return;
		}

		// Clicked on the "..". Equivalent to cd ..
		if (event.GetString() == "..") {
			listFiles->Clear();
			if (selectedItemData->GetLevel() == 1) {
				// We're back at the top level directory which is the media device itself
				// Don't append a dummy root, because we are the root !
				wxCommandEvent *commandEvent = new wxCommandEvent(wxEVT_MEDIAPLAYER_DIALOG_EVENT, MEDIAPLAYER_LIST_PARENT);
				commandEvent->SetInt(0);
				wxQueueEvent(eventHandler, commandEvent);
			}

			else {
				// Add a dummy root so we can return to the parent folder
				listFiles->Append("..", new MediaFileItem(0, MEDIA_FILE_TYPE::PHYSICALFOLDER, level - 1));
				
				// Get the child folders for the previous level
				wxCommandEvent* commandEvent = new wxCommandEvent(wxEVT_MEDIAPLAYER_DIALOG_EVENT, MEDIAPLAYER_LIST_PARENT);
				commandEvent->SetInt(0);
				wxQueueEvent(eventHandler, commandEvent);
			}
		}

		else {
			// Clicked on an item
			if (selectedItemData->GetMediaType() != MEDIA_FILE_TYPE::MUSICTRACK) {
				// Clear the list box and add a dummy root
				listFiles->Clear();
				listFiles->Append("..", new MediaFileItem(0, mediaType, level + 1));
				
				// Get the children of this folder
				wxCommandEvent* commandEvent = new wxCommandEvent(wxEVT_MEDIAPLAYER_DIALOG_EVENT, MEDIAPLAYER_LIST_CHILD);
				commandEvent->SetInt(fileId);
				wxQueueEvent(eventHandler, commandEvent);
			}
			else {
				// Play the music track
				wxCommandEvent* commandEvent = new wxCommandEvent(wxEVT_MEDIAPLAYER_DIALOG_EVENT, MEDIAPLAYER_FILE_PLAY);
				commandEvent->SetInt(fileId);
				wxQueueEvent(eventHandler, commandEvent);
			}
		}
	}
}

// Context Menu for the ListCtrl
void MediaPlayerDialog::OnRightClick(wxMouseEvent& event) {
	int itemNumber = listFiles->GetSelection();
	int contextMenuId;
	wxListBox* listBox = wxDynamicCast(event.GetEventObject(), wxListBox);
	MediaFileItem* selectedItemData = NULL;

	if (listBox != NULL && itemNumber != wxNOT_FOUND) {
		selectedItemData = static_cast<MediaFileItem*>(listBox->GetClientObject(itemNumber));

		if (selectedItemData->GetMediaType() == MEDIA_FILE_TYPE::MUSICTRACK) {
			wxMenu menu;
			menu.Append(MEDIAPLAYER_FILE_PLAY, "&Play");

			contextMenuId = listFiles->GetPopupMenuSelectionFromUser(menu);
			
			if (contextMenuId == MEDIAPLAYER_FILE_PLAY) {
				wxCommandEvent* commandEvent = new wxCommandEvent(wxEVT_MEDIAPLAYER_DIALOG_EVENT, MEDIAPLAYER_FILE_PLAY);
				commandEvent->SetInt(selectedItemData->GetId());
				wxQueueEvent(eventHandler, commandEvent);
			}
		}
		else if (selectedItemData->GetMediaType() == MEDIA_FILE_TYPE::RADIO) {
			wxMenu menu;
			menu.Append(MEDIAPLAYER_FILE_PLAY, "&Select");
			menu.Append(MEDIAPLAYER_DELETE_PRESET, "&Delete");

			contextMenuId = listFiles->GetPopupMenuSelectionFromUser(menu); 
			
			if (contextMenuId == MEDIAPLAYER_FILE_PLAY) {
				wxCommandEvent* commandEvent = new wxCommandEvent(wxEVT_MEDIAPLAYER_DIALOG_EVENT, MEDIAPLAYER_SELECT_PRESET);
				commandEvent->SetInt(selectedItemData->GetId());
				wxQueueEvent(eventHandler, commandEvent);
			}
			else if (contextMenuId == MEDIAPLAYER_DELETE_PRESET) {
				wxCommandEvent* commandEvent = new wxCommandEvent(wxEVT_MEDIAPLAYER_DIALOG_EVENT, MEDIAPLAYER_DELETE_PRESET);
				commandEvent->SetInt(selectedItemData->GetLevel());
				wxQueueEvent(eventHandler, commandEvent);
			}
		}
	}
}

void MediaPlayerDialog::OnZone0VolumeChanged(wxCommandEvent& event) {
	wxCommandEvent* commandEvent = new wxCommandEvent(wxEVT_MEDIAPLAYER_DIALOG_EVENT, MEDIAPLAYER_ZONE0);
	commandEvent->SetInt(sliderZone0Volume->GetValue());
	wxQueueEvent(eventHandler, commandEvent);
}

// BUG BUG Much of this will break if the labels change (eg. localization)
// Handle changes from the wxChoice control used to select different media sources
void MediaPlayerDialog::OnSourceChanged(wxCommandEvent& event) {
	wxString selectedSource = choiceSource->GetString(event.GetSelection());

	wxCommandEvent *commandEvent;
	commandEvent = new wxCommandEvent(wxEVT_MEDIAPLAYER_DIALOG_EVENT, MEDIAPLAYER_SOURCE);
	commandEvent->SetString(selectedSource);
	wxQueueEvent(eventHandler, commandEvent);
}

void MediaPlayerDialog::OnZone1VolumeChanged(wxCommandEvent& event) {
	wxCommandEvent *commandEvent = new wxCommandEvent(wxEVT_MEDIAPLAYER_DIALOG_EVENT, MEDIAPLAYER_ZONE1);
	commandEvent->SetInt(sliderZone1Volume->GetValue());
	wxQueueEvent(eventHandler, commandEvent);
}

void MediaPlayerDialog::OnBalanceChanged(wxCommandEvent& event) {
	wxCommandEvent *commandEvent = new wxCommandEvent(wxEVT_MEDIAPLAYER_DIALOG_EVENT, MEDIAPLAYER_BALANCE);
	commandEvent->SetInt(sliderBalance->GetValue());
	wxQueueEvent(eventHandler, commandEvent);
}

void MediaPlayerDialog::OnTrebleChanged(wxCommandEvent& event) {
	wxCommandEvent *commandEvent = new wxCommandEvent(wxEVT_MEDIAPLAYER_DIALOG_EVENT, MEDIAPLAYER_TREBLE);
	commandEvent->SetInt(sliderTreble->GetValue());
	wxQueueEvent(eventHandler, commandEvent);
}

void MediaPlayerDialog::OnMidrangeChanged(wxCommandEvent& event) {
	wxCommandEvent *commandEvent = new wxCommandEvent(wxEVT_MEDIAPLAYER_DIALOG_EVENT, MEDIAPLAYER_MIDRANGE);
	commandEvent->SetInt(sliderMidrange->GetValue());
	wxQueueEvent(eventHandler, commandEvent);
}

void MediaPlayerDialog::OnBassChanged(wxCommandEvent& event) {
	wxCommandEvent *commandEvent = new wxCommandEvent(wxEVT_MEDIAPLAYER_DIALOG_EVENT, MEDIAPLAYER_BASS);
	commandEvent->SetInt(sliderBass->GetValue());
	wxQueueEvent(eventHandler, commandEvent);
}

void MediaPlayerDialog::OnButtonReverse(wxCommandEvent& event) {
	wxCommandEvent *commandEvent = new wxCommandEvent(wxEVT_MEDIAPLAYER_DIALOG_EVENT, MEDIAPLAYER_PREVIOUS_TRACK);
	commandEvent->SetString(_T("Previous"));
	wxQueueEvent(eventHandler, commandEvent);
}

void MediaPlayerDialog::OnButtonPause(wxCommandEvent& event) {
	wxCommandEvent *commandEvent = new wxCommandEvent(wxEVT_MEDIAPLAYER_DIALOG_EVENT, MEDIAPLAYER_PAUSE);
	commandEvent->SetString(buttonPause->GetLabel());
	wxQueueEvent(eventHandler, commandEvent);
}

void MediaPlayerDialog::OnButtonForward(wxCommandEvent& event) {
	wxCommandEvent* commandEvent = new wxCommandEvent(wxEVT_MEDIAPLAYER_DIALOG_EVENT, MEDIAPLAYER_NEXT_TRACK);
	commandEvent->SetString(_T("Next"));
	wxQueueEvent(eventHandler, commandEvent);
}

//void MediaPlayerDialog::OnCheckShuffle(wxCommandEvent& event) {
//	wxCommandEvent *commandEvent = new wxCommandEvent(wxEVT_MEDIAPLAYER_DIALOG_EVENT, MEDIAPLAYER_SHUFFLE);
//	commandEvent->SetInt(checkShuffle->IsChecked());
//	wxQueueEvent(eventHandler, commandEvent);
//}

//void MediaPlayerDialog::OnCheckRepeat(wxCommandEvent& event) {
//	wxCommandEvent *commandEvent = new wxCommandEvent(wxEVT_MEDIAPLAYER_DIALOG_EVENT, MEDIAPLAYER_REPEAT);
//	commandEvent->SetInt(checkRepeat->IsChecked());
//	wxQueueEvent(eventHandler, commandEvent);
//}

void MediaPlayerDialog::OnButtonMute(wxCommandEvent& event) {
	wxCommandEvent *commandEvent = new wxCommandEvent(wxEVT_MEDIAPLAYER_DIALOG_EVENT, MEDIAPLAYER_MUTE);
	commandEvent->SetString(buttonMute->GetLabel());
	wxQueueEvent(eventHandler, commandEvent);
}

void MediaPlayerDialog::OnButtonPower(wxCommandEvent& event) {
	wxCommandEvent *commandEvent = new wxCommandEvent(wxEVT_MEDIAPLAYER_DIALOG_EVENT, MEDIAPLAYER_ON);
	commandEvent->SetString(buttonPower->GetLabel());
	wxQueueEvent(eventHandler, commandEvent);
}

// Setters invoked to keep dialog in synch with responses received from the media player
// Eg. the media player could be simultaneously controlled by its own remote control, 
// so we monitor the responses and ensure we display the same values.

void MediaPlayerDialog::SetZone0Volume(const unsigned int volume) {
	sliderZone0Volume->SetValue(volume);
}

void MediaPlayerDialog::SetZone1Volume(const unsigned int volume) {
	sliderZone1Volume->SetValue(volume);
}

void MediaPlayerDialog::SetBalance(const int balance) {
	sliderBalance->SetValue(balance);
}

void MediaPlayerDialog::SetBass(const unsigned int bass) {
	sliderBass->SetValue(bass);
}

void MediaPlayerDialog::SetMidrange(const unsigned int midrange) {
	sliderMidrange->SetValue(midrange);
}

void MediaPlayerDialog::SetTreble(const unsigned int treble) {
	sliderTreble->SetValue(treble);
}

void MediaPlayerDialog::SetZone0Name(const wxString &name) {
	sizerZone0Volume->GetStaticBox()->SetLabel(name.Capitalize() + " volume");
}

void MediaPlayerDialog::SetZone1Name(const wxString &name) {
	sizerZone1Volume->GetStaticBox()->SetLabel(name.Capitalize() + " volume");
}

void MediaPlayerDialog::SetStatus(const wxString &statusMessage) {
	labelMediaPlayerStatus->SetLabel(statusMessage);
}

//void MediaPlayerDialog::SetShuffle(const bool shuffleState) {
//	checkShuffle->SetValue(shuffleState);
//}

//void MediaPlayerDialog::SetRepeat(const bool repeatState) {
//	checkRepeat->SetValue(repeatState);
//}

void MediaPlayerDialog::SetGaugeRange(const unsigned int seconds) {
	gaugeProgress->SetRange(seconds);
	gaugeProgress->Refresh();
}

void MediaPlayerDialog::SetGaugeValue(const unsigned int elapsedTime) {
	gaugeProgress->SetValue(elapsedTime);
	gaugeProgress->Refresh();
}

void MediaPlayerDialog::SetPlayState(const bool playingState) {
	buttonPause->SetLabelText(playingState ? "Pause" : "Play");
}

void MediaPlayerDialog::SetPowerState(const bool powerState) {
	buttonPower->SetLabel(powerState ? "Off" : "On");
	buttonPause->Enable(powerState);
	buttonForward->Enable(powerState);
	buttonReverse->Enable(powerState);
	buttonMute->Enable(powerState);
	sliderZone0Volume->Enable(powerState);
	sliderZone1Volume->Enable(powerState);
	sliderBalance->Enable(powerState);
	sliderBass->Enable(powerState);
	sliderMidrange->Enable(powerState);
	sliderTreble->Enable(powerState);
	choiceSource->Enable(powerState);
	if (!powerState) {
		labelMediaPlayerStatus->SetLabel("Not connected");
		gaugeProgress->SetValue(0);
	}
}

void MediaPlayerDialog::SetMuteState(const bool muteState) {
	buttonMute->SetLabel(muteState ? "Unmute" : "Mute");
}

void MediaPlayerDialog::ClearSource(void) {
	choiceSource->Clear();
}

void MediaPlayerDialog::AddSource(const wxString &sourceName) {
	choiceSource->Append(sourceName);
}

void MediaPlayerDialog::SetSource(const wxString &sourceName) {
	int selectedItem;
	selectedItem = choiceSource->FindString(sourceName);
	if (selectedItem == wxNOT_FOUND) {
		return;
	}

	choiceSource->SetSelection(selectedItem);
	
	// Pause/Play, Shuffle & Repeat only applicable to USB/MTP/iPod devices
	if (choiceSource->GetString(choiceSource->GetSelection()).Lower() == "usb") {
		buttonPause->Enable(true);
		buttonPause->SetLabel("Pause");
		//checkRepeat->Enable(true);
		//checkShuffle->Enable(true);
	}
	
	// For radio, instead of Pause/Play we use the same button to save presets
	else if ((choiceSource->GetString(choiceSource->GetSelection()).Lower() == "am") ||
		(choiceSource->GetString(choiceSource->GetSelection()).Lower() == "fm")) {
		buttonPause->Enable(true);
		buttonPause->SetLabel("Save"); // Save current station as a preset;
	}
	else {
		buttonPause->Enable(false);
	}

	// Only a usb media device or am/fm radio has the concept of next, previous
	if ((choiceSource->GetString(choiceSource->GetSelection()).Lower() == "am") ||
		(choiceSource->GetString(choiceSource->GetSelection()).Lower() == "fm") ||
		(choiceSource->GetString(choiceSource->GetSelection()).Lower() == "usb")) {
		buttonForward->Enable(true);
		buttonReverse->Enable(true);
	}
	else {
		buttonForward->Enable(false);
		buttonReverse->Enable(false);
	}
}

// Media File Stuff

// Called when we receive an item of type MTPFOLDER (eg. root)
void MediaPlayerDialog::SetRootFolder(const wxString &folderName) {
	listFiles->Clear();
	listFiles->Append(folderName, new MediaFileItem(0, MEDIA_FILE_TYPE::MTPFOLDER, 0));
}

void MediaPlayerDialog::AppendFolder(const wxString &folderName, const MEDIA_FILE_TYPE folderType, const int folderId) {
	// Called when we receive an item of type PHYSICAL_FOLDER or VIRTUAL_FOLDER
	// Use the dummy root, to get the current folder heirarchy level
	int itemNumber = listFiles->FindString("..");
	MediaFileItem *selectedItemData = NULL;

	if (itemNumber != wxNOT_FOUND) {
		selectedItemData = static_cast<MediaFileItem*>(listFiles->GetClientObject(itemNumber));
		int level = selectedItemData->GetLevel();
		listFiles->Append(folderName, new MediaFileItem(folderId, folderType, level));
	}
}

void MediaPlayerDialog::AppendFile(const wxString &itemName, const int itemId) {
	// Called when we receive an item of type MUSIC_TRACK
	// Use the dummy root, to get the current folder heirarchy
	int itemNumber = listFiles->FindString("..");
	MediaFileItem *selectedItemData = NULL;

	if (itemNumber != wxNOT_FOUND) {
		selectedItemData = static_cast<MediaFileItem*>(listFiles->GetClientObject(itemNumber));
		int level = selectedItemData->GetLevel();
		listFiles->Append(itemName, new MediaFileItem(itemId, MEDIA_FILE_TYPE::MUSICTRACK, level));
	}
}

// Called whenever a non usb device is selected
void MediaPlayerDialog::ClearFileList(void) {
	listFiles->Clear();
}

// Append a preset radio statio to the list control.
void MediaPlayerDialog::AppendRadioStation(const wxString &radioStation, const int frequency, const int index) {
	// ClientData is used to contain the preset radio station metadata
	// folderId contains the frequency, level contains the index
	listFiles->Append(radioStation, new MediaFileItem(frequency, MEDIA_FILE_TYPE::RADIO, index));
}

// Called when a usb device is selected. Gets the device name
// BUG BUG UNUSED
void MediaPlayerDialog::RequestRootFolder(void) {
	wxCommandEvent *commandEvent = new wxCommandEvent(wxEVT_MEDIAPLAYER_DIALOG_EVENT, MEDIAPLAYER_PREVIOUS_FOLDER);
	commandEvent->SetInt(0);
	wxQueueEvent(eventHandler, commandEvent);
}

// Called when double clicked on a folder, to retrieve child folders
// BUG BUG UNUSED
void MediaPlayerDialog::RequestChildFolder(int folderId) {
	wxCommandEvent *commandEvent = new wxCommandEvent(wxEVT_MEDIAPLAYER_DIALOG_EVENT, MEDIAPLAYER_NEXT_FOLDER);
	commandEvent->SetInt(folderId);
	wxQueueEvent(eventHandler, commandEvent);
}

// Called when double clicking on ".." to go back up one level in the folder heirarchy
// BUG BUG Unused
void MediaPlayerDialog::RequestParentFolder(int folderId) {
	wxCommandEvent *commandEvent = new wxCommandEvent(wxEVT_MEDIAPLAYER_DIALOG_EVENT, MEDIAPLAYER_PREVIOUS_FOLDER);
	commandEvent->SetInt(folderId);
	wxQueueEvent(eventHandler, commandEvent);
}

