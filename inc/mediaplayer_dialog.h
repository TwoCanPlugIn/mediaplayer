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

// The dialog form created by wxFormBuilder
#include "mediaplayer_dialogbase.h"

// For context menu
#include <wx/menu.h>

// For OpenCPN colour scheme
#include <ocpn_plugin.h>

// Apply my colours to support Day/Dusk/Night mode
struct MediaPlayerTheme {

	wxColour background;
	wxColour panelBackground;
	wxColour foreground;
	wxColour buttonBackground;
	wxColour buttonForeground;
	wxColour border;
	wxColour highlight;

	static MediaPlayerTheme GetTheme(PI_ColorScheme mode) {
		MediaPlayerTheme theme;
		if (mode == PI_GLOBAL_COLOR_SCHEME_DAY) {
			theme.background = wxColour(240, 240, 240); // V Light Grey
			theme.panelBackground = wxColour(255, 255, 255); // White
			theme.foreground = wxColour(0, 0, 0); // Black
			theme.buttonBackground = wxColour(225, 225, 225); // Light Grey
			theme.buttonForeground = wxColour(0, 0, 0);
			theme.border = wxColour(160, 160, 160); //  Grey
			theme.highlight = wxColour(0, 120, 215); // Blue
		}
		else { // Use the same theme for dusk & night
			theme.background = wxColour(20, 20, 20); // v. DARK GREY
			theme.panelBackground = wxColour(25, 25, 25);
			theme.foreground = wxColour(255, 50, 50); // Red
			theme.buttonBackground = wxColour(45, 45, 45); // Grey
			theme.buttonForeground = wxColour(255, 50, 50); // Red
			theme.border = wxColour(150, 30, 30); // Dark red
			theme.highlight = wxColour(255, 0, 0); // Red
		}
		return theme;
	}
};

// Was thinking of using relative paths for navigation
//#if (defined (__APPLE__) && defined (__MACH__))
//#include <experimental/filesystem>
//#else
//#include <filesystem>
//#endif


// The dialog events
extern const wxEventType wxEVT_MEDIAPLAYER_DIALOG_EVENT;
extern const int MEDIAPLAYER_ON;
extern const int MEDIAPLAYER_MUTE;
extern const int MEDIAPLAYER_NEXT_TRACK;
extern const int MEDIAPLAYER_PREVIOUS_TRACK;
extern const int MEDIAPLAYER_PAUSE;
extern const int MEDIAPLAYER_ZONE0;
extern const int MEDIAPLAYER_ZONE1;
extern const int MEDIAPLAYER_SOURCE;
extern const int MEDIAPLAYER_BASS;
extern const int MEDIAPLAYER_MIDRANGE;
extern const int MEDIAPLAYER_TREBLE;
extern const int MEDIAPLAYER_BALANCE;
extern const int MEDIAPLAYER_FILE_PLAY;
extern const int MEDIAPLAYER_SHUFFLE;
extern const int MEDIAPLAYER_REPEAT;
extern const int MEDIAPLAYER_NEXT_FOLDER;
extern const int MEDIAPLAYER_PREVIOUS_FOLDER;
extern const int MEDIAPLAYER_CURRENT_FOLDER;
extern const int MEDIAPLAYER_LIST_DEVICE;
extern const int MEDIAPLAYER_LIST_ROOT;
extern const int MEDIAPLAYER_LIST_CURRENT;
extern const int MEDIAPLAYER_LIST_PARENT;
extern const int MEDIAPLAYER_LIST_CHILD;
extern const int MEDIAPLAYER_SELECT_PRESET;
extern const int MEDIAPLAYER_DELETE_PRESET;


// The different folder types returned by the Fusion Media Player
// Wonder if there is a folder type for iPods ?
enum MEDIA_FILE_TYPE {
	VIRTUALFOLDER = 0x49, // & 0x40
	DEVICEFOLDER = 0x41,
	MTPFOLDER = 0x47,
	PHYSICALFOLDER = 0x4F,
	MUSICTRACK = 0x1F, // & 0x10
	MTPTRACK = 0x17, //
	// I'm using this to differentatiate radio stations
	RADIO = 0xFF
};

// Metadata used for list box items representing music folders, tracks or radio presets
class MediaFileItem : public wxClientData {
public:
	MediaFileItem(const int id, const MEDIA_FILE_TYPE type, const int level);
	~MediaFileItem();
	MEDIA_FILE_TYPE GetMediaType();
	int GetId();
	int GetLevel();

private:
	MEDIA_FILE_TYPE itemType;
	int itemId;
	int itemLevel;
};

class MediaPlayerDialog : public MediaPlayerDialogBase
{
	protected:
		void OnSourceChanged( wxCommandEvent& event) override;
		void OnZone0VolumeChanged( wxCommandEvent& event) override;
		void OnZone1VolumeChanged( wxCommandEvent& event) override;
		void OnBalanceChanged(wxCommandEvent& event) override;
		void OnTrebleChanged(wxCommandEvent& event) override;
		void OnMidrangeChanged(wxCommandEvent& event) override;
		void OnBassChanged(wxCommandEvent& event) override;
		//void OnCheckShuffle(wxCommandEvent& event) override;
		//void OnCheckRepeat(wxCommandEvent& event) override;
		void OnButtonReverse( wxCommandEvent& event) override;
		void OnButtonPause( wxCommandEvent& event) override;
		void OnButtonForward( wxCommandEvent& event) override;
		void OnButtonMute( wxCommandEvent& event) override;
		void OnButtonPower(wxCommandEvent& event) override;
		void OnFileSelected(wxCommandEvent& event) override;
		void OnRightClick(wxMouseEvent& event) override;
						
	public:
		MediaPlayerDialog( wxWindow* parent, wxEvtHandler *handler);
		~MediaPlayerDialog(void);
		// Setters
		void SetZone0Volume(const unsigned int volume);
		void SetZone1Volume(const unsigned int volume);
		void SetBalance(const int balance);
		void SetBass(const unsigned int bass);
		void SetMidrange(const unsigned int midrance);
		void SetTreble(const unsigned int treble);
		void SetZone0Name(const wxString &name);
		void SetZone1Name(const wxString &name);
		void SetStatus(const wxString &statusMessage);
		//void SetShuffle(bool shuffleSTate);
		//void SetRepeat(bool repeatState);
		void SetPlayState(bool playingState);
		void SetPowerState(const bool powerState);
		void SetMuteState(const bool muteState);
		void SetGaugeRange(const unsigned int trackLength);
		void SetGaugeValue(const unsigned int elapsedTime);
		// Used to manage the wxChoice control for selecting media sources
		void ClearSource(void);
		void SetSource(const wxString &sourceName);
		void AddSource(const wxString &sourceName);
		// Used to manage the media files list box
		void ClearFileList(void);
		void AppendFile(const wxString &itemName, const int itemId);
		void AppendFolder(const wxString &folderName, const MEDIA_FILE_TYPE folderType, const int folderId);
		void SetRootFolder(const wxString &folderName);
		void AppendRadioStation(const wxString &radioStation, const int frequency, const int index);
		// Currently unused - Could abstract the command events
		void RequestRootFolder(void);
		void RequestChildFolder(int folderId);
		void RequestParentFolder(int folderId);
		// Day/Dusk/Night colour schemes
		void SetColourScheme(PI_ColorScheme cs);
		void ApplyColourScheme(wxWindow *windows, MediaPlayerTheme them);
		void ApplyDimeColourScheme(wxWindow* window);
		
	private:
		wxEvtHandler *eventHandler;
		
};
#endif
