// Copyright(C) 2018-2020 by Steven Adler
//
// This file is part of Media Player Control plugin for OpenCPN.
//
// Media Player Control plugin for OpenCPN is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Media Player Control plugin for OpenCPN is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with the Media Player Control plugin for OpenCPN. If not, see <https://www.gnu.org/licenses/>.

#ifndef MEDIAPLAYER_CONTROL_PANEL_H
#define MEDIAPLAYER_CONTROL_PANEL_H

#include "mediaplayer_dialogbase.h"
// for context menu
#include <wx/menu.h>
// for splitting relative paths
#include <wx/tokenzr.h>
// not sure where an unordered map is used
#include <unordered_map>
// for navigation
#include <deque>
// To determine relative paths
#if (defined (__APPLE__) && defined (__MACH__))
#include <experimental/filesystem>
#else
#include <filesystem>
#endif
//namespace fs = std::filesystem;

extern bool mediaPlayerControlPanelVisible;
extern bool isMuted;
extern bool isOn;
extern bool isPlaying;
extern double frequency;
extern wxString mediaSource;

extern const wxEventType wxEVT_MEDIAPLAYER_DIALOG_EVENT;
extern const int MEDIAPLAYER_ON;
extern const int MEDIAPLAYER_MUTE;
extern const int MEDIAPLAYER_NEXT;
extern const int MEDIAPLAYER_PREVIOUS;
extern const int MEDIAPLAYER_PAUSE;
extern const int MEDIAPLAYER_ZONE0;
extern const int MEDIAPLAYER_ZONE1;
extern const int MEDIAPLAYER_SOURCE;
extern const int MEDIAPLAYER_BASS;
extern const int MEDIAPLAYER_MIDRANGE;
extern const int MEDIAPLAYER_TREBLE;
extern const int MEDIAPLAYER_BALANCE;
extern const int MEDIAPLAYER_NEXT_FOLDER;
extern const int MEDIAPLAYER_PREVIOUS_FOLDER;
extern const int MEDIAPLAYER_CURRENT_FOLDER;
extern const int MEDIAPLAYER_FILE_PLAY;
extern const int MEDIAPLAYER_SHUFFLE;
extern const int MEDIAPLAYER_REPEAT;
extern const int MEDIAPLAYER_LIST_CURRENT;
extern const int MEDIAPLAYER_LIST_PARENT;
extern const int MEDIAPLAYER_LIST_CHILD;

#define ID_MENU_ITEM_PLAY 100

// A queue to handle navigation through albums and tracks
// Essentailly make the navigation operations synchronous
struct MediaNavigationStep {
	enum Direction {
		Up,
		Down
	};

	Direction direction;
	int itemId;
};

std::deque<MediaNavigationStep> navigationSteps;
bool navigationInProgress = false;

// The different folder types returned by the Fusion Media Player
typedef enum folderType {
	VIRTUALFOLDER = 0x49,
	DEVICEFOLDER = 0x41,
	MTPFOLDER = 0x47,
	PHYSICALFOLDER = 0x4F,
	MUSICTRACK = 0x1F
} MEDIA_FILE_TYPE;

// Client data used for list box items representing music folders & tracks
class MediaFileItem : public wxClientData {
public:
	MediaFileItem(int id, int type, int level);
	~MediaFileItem();
	int GetMediaType();
	int GetId();
	int GetLevel();

private:
	int itemType;
	int itemId;
	int itemLevel;
};

class MediaPlayerDialog : public MediaPlayerDialogBase
{
	protected:
		void OnSourceChanged( wxCommandEvent& event );
		void OnZone0VolumeChanged( wxCommandEvent& event );
		void OnZone1VolumeChanged( wxCommandEvent& event );
		void OnBalanceChanged(wxCommandEvent& event);
		void OnTrebleChanged(wxCommandEvent& event);
		void OnMidrangeChanged(wxCommandEvent& event);
		void OnBassChanged(wxCommandEvent& event);
		void OnCheckShuffle(wxCommandEvent& event);
		void OnCheckRepeat(wxCommandEvent& event);
		void OnButtonReverse( wxCommandEvent& event );
		void OnButtonPause( wxCommandEvent& event );
		void OnButtonForward( wxCommandEvent& event );
		void OnButtonMute( wxCommandEvent& event );
		void OnButtonPower(wxCommandEvent& event);
		void OnFileSelected(wxCommandEvent& event);
		// BUG BUG Used for debugging
		void OnButtonCurrent(wxCommandEvent& event);
		void OnButtonChild(wxCommandEvent& event);
		void OnButtonParent(wxCommandEvent& event);
				
	public:
		MediaPlayerDialog( wxWindow* parent, wxEvtHandler *handler);
		~MediaPlayerDialog(void);
		// Getters & Setters
		void SetZone0Volume(unsigned int volume);
		void SetZone1Volume(unsigned int volume);
		void SetBalance(int balance);
		void SetBass(unsigned int bass);
		void SetMidrange(unsigned int midrance);
		void SetTreble(unsigned int treble);
		void SetZone0Name(wxString name);
		void SetZone1Name(wxString name);
		void SetStatus(wxString statusMessage);
		void SetShuffle(bool shuffleSTate);
		void SetRepeat(bool repeatState);
		void SetPlayState(bool playingState);
		void SetPowerState(bool powerState);
		void SetMuteState(bool muteState);
		void SetGaugeRange(unsigned int trackLength);
		void SetGaugeValue(unsigned int elapsedTime);
		void SetSource(wxString sourceName);
		void ClearFileList(void);
		void RequestRootFolder(void);
		void RequestChildFolder(int folderId);
		void RequestParentFolder(int folderId);
		void AppendFile(wxString itemName, int itemId);
		void AppendFolder(wxString folderName, MEDIA_FILE_TYPE folderType, int folderId);
		void SetRootFolder(wxString folderName);

	private:
		wxEvtHandler *eventHandler;
		
};
#endif
