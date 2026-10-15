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
//
// BUG BUG To Do
// Pre-set radio stations, Settings (Zone & Device Names etc.), Bluetooth pairing with media devices.

#include "mediaplayer_dialog.h"

// BUG BUG DEBUG REMOVE
#include <wx/socket.h>
#include <wx/msgdlg.h>

extern wxDatagramSocket *debugSocket;
extern wxIPV4address addrPeer;
wxString msg;

// Media File/Folder Client Item Data used for folders & music tracks from media devices
MediaFileItem::MediaFileItem(int id, int type, int level) {
	itemType = type;
	itemId = id;
	itemLevel = level;
}

MediaFileItem::~MediaFileItem() {
}

int MediaFileItem::GetId() {
	return itemId;
}

int MediaFileItem::GetMediaType() {
	return itemType;
}

int MediaFileItem::GetLevel() {
	return itemLevel;
}

// The plugin dialog, inherits from a base class developed using wxFormBuilder
MediaPlayerDialog::MediaPlayerDialog( wxWindow* parent, wxEvtHandler *handler) : MediaPlayerDialogBase(parent) {
	eventHandler = handler;
	Layout();
}

MediaPlayerDialog::~MediaPlayerDialog(void) {
}

// Event Handlers
// Debugging
void MediaPlayerDialog::OnButtonCurrent(wxCommandEvent& event) {
	wxCommandEvent *commandEvent = new wxCommandEvent(wxEVT_MEDIAPLAYER_DIALOG_EVENT, MEDIAPLAYER_LIST_CURRENT);
	wxQueueEvent(eventHandler, commandEvent);
}

void MediaPlayerDialog::OnButtonChild(wxCommandEvent& event) {
	wxCommandEvent *commandEvent = new wxCommandEvent(wxEVT_MEDIAPLAYER_DIALOG_EVENT, MEDIAPLAYER_LIST_CHILD);
	wxQueueEvent(eventHandler, commandEvent);
}

void MediaPlayerDialog::OnButtonParent(wxCommandEvent& event) {
	wxCommandEvent *commandEvent = new wxCommandEvent(wxEVT_MEDIAPLAYER_DIALOG_EVENT, MEDIAPLAYER_LIST_PARENT);
	wxQueueEvent(eventHandler, commandEvent);
}

// Clicking on a track in the media list box
void MediaPlayerDialog::OnFileSelected(wxCommandEvent& event) {

	int itemNumber = event.GetSelection();
	wxListBox *listBox = wxDynamicCast(event.GetEventObject(), wxListBox);
	MediaFileItem *selectedItemData = NULL;

	if (listBox != NULL && itemNumber != wxNOT_FOUND) {
		selectedItemData = static_cast<MediaFileItem*>(listBox->GetClientObject(itemNumber));

		int fileId = selectedItemData->GetId();
		int level = selectedItemData->GetLevel();
		int mediaType = selectedItemData->GetMediaType();

		if (event.GetString() == "..") {

			wxMessageBox(wxString::Format("On cd .. selected, Level: %d", level));

			if (selectedItemData->GetLevel() == 1) {

				// We're back at the top level directory which is the media device itself
				listFiles->Clear();
				// Don't append a dummy root, because we are the root !
				// Get the device name to display
				RequestRootFolder();
			}

			else {

				listFiles->Clear();
				// Add a dummy root for the previous level
				listFiles->Append("..", new MediaFileItem(0, MEDIA_FILE_TYPE::PHYSICALFOLDER, level - 1));
				// Get the child folders from the previous heirachy level
				RequestChildFolder(0);
			}
		}

		else {
			// Clicked on a folder name
			if (selectedItemData->GetMediaType() != MEDIA_FILE_TYPE::MUSICTRACK) {

				wxMessageBox(wxString::Format("Level: %d, Folder Id: %d", level, fileId));

				// Clear the list box and add the dummy root
				listFiles->Clear();
				listFiles->Append("..", new MediaFileItem(0, mediaType, level + 1));
				// Get the child folders
				RequestChildFolder(fileId);
			}
		}
	}
}


// Handle changes from the radio buttons used to select different media sources
void MediaPlayerDialog::OnSourceChanged(wxCommandEvent& event) {
	wxString selectedSource = radioBoxSource->GetString(event.GetSelection());
	wxCommandEvent *commandEvent;
	commandEvent = new wxCommandEvent(wxEVT_MEDIAPLAYER_DIALOG_EVENT, MEDIAPLAYER_SOURCE);
	commandEvent->SetString(selectedSource);
	wxQueueEvent(eventHandler, commandEvent);
	// Only a usb media device has the concept of pause/play,shuffle & repeat
	// BUG BUG is this necessary as the setters also perform this
	// BUG BUG Impact of translations on selections
	if (selectedSource.Lower() == "usb") {
		buttonPause->Enable(true);
		checkRepeat->Enable(true);
		checkShuffle->Enable(true);
	}
	else {
		buttonPause->Enable(false);
		checkRepeat->Enable(false);
		checkShuffle->Enable(false);
	}

	// Only a usb media device or am/fm radio has the concept of next, previous
	if ((selectedSource.Lower() == "am") || (selectedSource.Lower() == "fm") ||
		(selectedSource.Lower() == "usb")) {
		buttonForward->Enable(true);
		buttonReverse->Enable(true);
	}
	else {
		buttonForward->Enable(false);
		buttonReverse->Enable(false);
	}
}


// Context Menu for music tracks
/*
void MediaPlayerDialog::OnItemContextMenu(wxTreeEvent& event) {
	MediaFileItem *itemData;
	itemData = (MediaFileItem *)mediaTreeList->GetItemData(event.GetItem());
	if (itemData->GetItemType() == MEDIA_FILE_TYPE::MUSICTRACK) {
		wxMenu menu;
		menu.Append(ID_MENU_ITEM_PLAY, "&Play");

		if (mediaTreeList->GetPopupMenuSelectionFromUser(menu) == ID_MENU_ITEM_PLAY) {
			wxCommandEvent *commandEvent = new wxCommandEvent(wxEVT_MEDIAPLAYER_DIALOG_EVENT, MEDIAPLAYER_FILE_PLAY);
			commandEvent->SetString(std::to_string(itemData->GetItemId()));
			wxQueueEvent(eventHandler, commandEvent);
		}
	}
}

*/

void MediaPlayerDialog::OnZone0VolumeChanged(wxCommandEvent& event) {
	wxCommandEvent *commandEvent = new wxCommandEvent(wxEVT_MEDIAPLAYER_DIALOG_EVENT, MEDIAPLAYER_ZONE0);
	commandEvent->SetInt(sliderZone0Volume->GetValue());
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
	wxCommandEvent *commandEvent = new wxCommandEvent(wxEVT_MEDIAPLAYER_DIALOG_EVENT, MEDIAPLAYER_PREVIOUS);
	commandEvent->SetString(_T("Previous"));
	wxQueueEvent(eventHandler, commandEvent);
}

void MediaPlayerDialog::OnButtonPause(wxCommandEvent& event) {
	wxCommandEvent *commandEvent = new wxCommandEvent(wxEVT_MEDIAPLAYER_DIALOG_EVENT, MEDIAPLAYER_PAUSE);
	commandEvent->SetString(buttonPause->GetLabel());
	wxQueueEvent(eventHandler, commandEvent);
	isPlaying = !isPlaying;
	// Let the setter do it's stuff
	//buttonPause->SetLabel(isPlaying ? "Pause" :"Play");
} 

void MediaPlayerDialog::OnCheckShuffle(wxCommandEvent& event) {
	wxCommandEvent *commandEvent = new wxCommandEvent(wxEVT_MEDIAPLAYER_DIALOG_EVENT, MEDIAPLAYER_SHUFFLE);
	commandEvent->SetInt(checkShuffle->IsChecked());
	wxQueueEvent(eventHandler, commandEvent);
}

void MediaPlayerDialog::OnCheckRepeat(wxCommandEvent& event) {
	wxCommandEvent *commandEvent = new wxCommandEvent(wxEVT_MEDIAPLAYER_DIALOG_EVENT, MEDIAPLAYER_REPEAT);
	commandEvent->SetInt(checkRepeat->IsChecked());
	wxQueueEvent(eventHandler, commandEvent);
}

void MediaPlayerDialog::OnButtonForward(wxCommandEvent& event) {
	wxCommandEvent *commandEvent = new wxCommandEvent(wxEVT_MEDIAPLAYER_DIALOG_EVENT, MEDIAPLAYER_NEXT);
	commandEvent->SetString(_T("Next"));
	wxQueueEvent(eventHandler, commandEvent);
}

void MediaPlayerDialog::OnButtonMute(wxCommandEvent& event) {
	wxCommandEvent *commandEvent = new wxCommandEvent(wxEVT_MEDIAPLAYER_DIALOG_EVENT, MEDIAPLAYER_MUTE);
	commandEvent->SetString(buttonMute->GetLabel());
	wxQueueEvent(eventHandler, commandEvent);
	isMuted = !isMuted;
	buttonMute->SetLabel(isMuted ? "Unmute" : "Mute");
}

void MediaPlayerDialog::OnButtonPower(wxCommandEvent& event) {
	wxCommandEvent *commandEvent = new wxCommandEvent(wxEVT_MEDIAPLAYER_DIALOG_EVENT, MEDIAPLAYER_ON);
	commandEvent->SetString(buttonPower->GetLabel());
	wxQueueEvent(eventHandler, commandEvent);
	isOn = !isOn;
	buttonPower->SetLabel(isOn ? "Off" : "On");
	// Set other control states as when the device is off, we get no notifications
	if (!isOn) {
		buttonPause->Enable(false);
		buttonForward->Enable(false);
		buttonReverse->Enable(false);
		labelMediaPlayerStatus->SetLabel(wxEmptyString);
		gaugeProgress->SetValue(0);
	}
}

// Setters invoked to keep dialog in synch with responses received from the media player
// Eg. the media player could be simultaneously controlled by its own remote control, 
// so we monitor the responses and ensure we display the same values.

void MediaPlayerDialog::SetZone0Volume(unsigned int volume) {
	sliderZone0Volume->SetValue(volume);
}

void MediaPlayerDialog::SetZone1Volume(unsigned int volume) {
	sliderZone1Volume->SetValue(volume);
}

void MediaPlayerDialog::SetBalance(int balance) {
	sliderBalance->SetValue(balance);
}

void MediaPlayerDialog::SetBass(unsigned int bass) {
	sliderBass->SetValue(bass);
}

void MediaPlayerDialog::SetMidrange(unsigned int midrange) {
	sliderMidrange->SetValue(midrange);
}

void MediaPlayerDialog::SetTreble(unsigned int treble) {
	sliderTreble->SetValue(treble);
}

void MediaPlayerDialog::SetZone0Name(wxString name) {
	sizerZone0Volume->GetStaticBox()->SetLabel(name.Capitalize() + " volume");
}

void MediaPlayerDialog::SetZone1Name(wxString name) {
	sizerZone1Volume->GetStaticBox()->SetLabel(name.Capitalize() + " volume");
}

void MediaPlayerDialog::SetStatus(wxString statusMessage) {
	labelMediaPlayerStatus->SetLabel(statusMessage);
}

void MediaPlayerDialog::SetShuffle(bool shuffleState) {
	checkShuffle->SetValue(shuffleState);
}

void MediaPlayerDialog::SetRepeat(bool repeatState) {
	checkRepeat->SetValue(repeatState);
}

void MediaPlayerDialog::SetGaugeRange(unsigned int seconds) {
	gaugeProgress->SetRange(seconds);
	gaugeProgress->Refresh();
}

void MediaPlayerDialog::SetGaugeValue(unsigned int elapsedTime) {
	gaugeProgress->SetValue(elapsedTime);
	gaugeProgress->Refresh();
}

void MediaPlayerDialog::SetPlayState(bool playingState) {
	isPlaying = playingState;
	buttonPause->SetLabelText(isPlaying ? "Pause" : "Play");
	// By definition,if we are playing, we are on.
	// Perhaps unit has been turned on by another remote
	SetPowerState(isPlaying);
}

void MediaPlayerDialog::SetPowerState(bool powerState) {
	isOn = powerState;
	buttonPower->SetLabel(isOn ? "Off" : "On");
	if (!isOn) {
		buttonPause->Enable(false);
		buttonForward->Enable(false);
		buttonReverse->Enable(false);
		labelMediaPlayerStatus->SetLabel(wxEmptyString);
		gaugeProgress->SetValue(0);
	}
	else {
		buttonPause->Enable(true);
		buttonForward->Enable(true);
		buttonReverse->Enable(true);

	}
}

void MediaPlayerDialog::SetMuteState(bool muteState) {
	isMuted = muteState;
	buttonMute->SetLabel(isMuted ? "Unmute" : "Mute");
}

void MediaPlayerDialog::SetSource(wxString sourceName) {
	for (size_t i = 0; i < radioBoxSource->GetCount(); i++) {
		if (radioBoxSource->GetString(i).Lower() == sourceName) {
			if (radioBoxSource->GetSelection() != i) {
				radioBoxSource->SetSelection(i);
			}
		}
	}
	// Pause/Play, Shuffle & Repeat only applicable to USB/MTP/iPod devices
	if (radioBoxSource->GetString(radioBoxSource->GetSelection()).Lower() == "usb") {
		buttonPause->Enable(true);
		checkRepeat->Enable(true);
		checkShuffle->Enable(true);
	}
	else {
		buttonPause->Enable(false);
		checkRepeat->Enable(false);
		checkShuffle->Enable(false);
	}

	// Only a usb media device or am/fm radio has the concept of next, previous
	if ((radioBoxSource->GetString(radioBoxSource->GetSelection()).Lower() == "am") ||
		(radioBoxSource->GetString(radioBoxSource->GetSelection()).Lower() == "fm") ||
		(radioBoxSource->GetString(radioBoxSource->GetSelection()).Lower() == "usb")) {
		buttonForward->Enable(true);
		buttonReverse->Enable(true);
	}
	else {
		buttonForward->Enable(false);
		buttonReverse->Enable(false);
	}
}

// Media File Stuff
void MediaPlayerDialog::AppendFolder(wxString folderName, MEDIA_FILE_TYPE folderType, int folderId) {
	// Called when we receive a folder of type PHYSICAL_FOLDER. VIRTUAL_FOLDER
	// Use the dummy root, to get the current folder heirarchy level
	int itemNumber = listFiles->FindString("..");
	MediaFileItem *selectedItemData = NULL;

	if (itemNumber != wxNOT_FOUND) {
		selectedItemData = static_cast<MediaFileItem*>(listFiles->GetClientObject(itemNumber));
		int level = selectedItemData->GetLevel();
		listFiles->Append(folderName, new MediaFileItem(folderId, folderType, level));
	}
}

// Called when we receive a folder of type MTPFOLDER (eg. root)
void MediaPlayerDialog::SetRootFolder(wxString folderName) {
	listFiles->Clear();
	listFiles->Append(folderName, new MediaFileItem(0, MEDIA_FILE_TYPE::MTPFOLDER, 0));
}

void MediaPlayerDialog::AppendFile(wxString itemName, int itemId) {
	// Called when we receive a folder of type MUSIC_TRACK
	// Use the dummy root, to get the folder heirarchy level
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

// Called when a usb device is selected. Gets the device name
void MediaPlayerDialog::RequestRootFolder(void) {
	wxCommandEvent *commandEvent = new wxCommandEvent(wxEVT_MEDIAPLAYER_DIALOG_EVENT, MEDIAPLAYER_PREVIOUS_FOLDER);
	commandEvent->SetInt(0);
	wxQueueEvent(eventHandler, commandEvent);
}

// Called when double clicked on a folder, to retrieve child folders
void MediaPlayerDialog::RequestChildFolder(int folderId) {
	wxCommandEvent *commandEvent = new wxCommandEvent(wxEVT_MEDIAPLAYER_DIALOG_EVENT, MEDIAPLAYER_NEXT_FOLDER);
	commandEvent->SetInt(folderId);
	wxQueueEvent(eventHandler, commandEvent);
}

// Called when double clicking on ".." to go back up one level in the folder heirarchy
void MediaPlayerDialog::RequestParentFolder(int folderId) {
	wxCommandEvent *commandEvent = new wxCommandEvent(wxEVT_MEDIAPLAYER_DIALOG_EVENT, MEDIAPLAYER_PREVIOUS_FOLDER);
	commandEvent->SetInt(folderId);
	wxQueueEvent(eventHandler, commandEvent);
	//root["entertainment"]["device"]["media"]["request"] = FOLDER_REQUEST::PREVIOUS_FOLDER;
	//root["entertainment"]["device"]["media"]["folderid"] = 0; // This seems to always be zero
	//writer.Write(root, message_body);
	//SendPluginMessage(_T("TWOCAN_MEDIA_REQUEST"), message_body);
}
