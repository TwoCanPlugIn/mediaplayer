// Copyright(C) 2021-2026 by Steven Adler
//
// This file is part of Fusion Media Player plugin for OpenCPN.
//
// Fusion Media Player plugin for OpenCPN is free software: you can 
// redistribute it and/or modify  it under the terms of the GNU General 
// Public License as published by the Free Software Foundation, either version 3 
// of the License, or (at your option) any later version.
//
// Fusion Media Player plugin for OpenCPN is distributed in the hope that 
// it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty 
// of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
// See the GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with the Fusion Media Player plugin for OpenCPN. 
// If not, see <https://www.gnu.org/licenses/>.
//
// NMEA2000® is a registered trademark of the National Marine Electronics Association
// Fusion® is a registered trademark of Garmin Ltd.


#ifndef MEDIAPLAYERCONTROL_PLUGIN_H
#define MEDIAPLAYERCONTROL_PLUGIN_H

// Pre compiled headers 
#include "wx/wxprec.h"

#ifndef WX_PRECOMP
      #include <wx/wx.h>
#endif

// wxWidgets Requirements
// Advanced User Interface
#include "wx/aui/aui.h"
#include <wx/aui/framemanager.h>

// Configuration
#include <wx/fileconf.h>

// STL
#include <map>
#include <cstring>

// Defines version numbers, names etc. for this plugin
// This is automagically constructed via version.h.in from CMakeLists.txt
#include "config.h"

// OpenCPN include file
#include "ocpn_plugin.h"

// Media Player Control Dialog
#include "mediaplayer_dialog.h"

// Media Player Settings Dialog
#include "mediaplayer_options.h"

// Plugin receives events from the Media Player Control Window
const wxEventType wxEVT_MEDIAPLAYER_DIALOG_EVENT = wxNewEventType();
const int MEDIAPLAYER_ON = wxID_HIGHEST + 1;
const int MEDIAPLAYER_MUTE = wxID_HIGHEST + 2;
const int MEDIAPLAYER_NEXT_TRACK = wxID_HIGHEST + 3;
const int MEDIAPLAYER_PREVIOUS_TRACK = wxID_HIGHEST + 4;
const int MEDIAPLAYER_PAUSE = wxID_HIGHEST + 5;
const int MEDIAPLAYER_ZONE0 = wxID_HIGHEST + 6;
const int MEDIAPLAYER_ZONE1 = wxID_HIGHEST + 7;
const int MEDIAPLAYER_SOURCE = wxID_HIGHEST + 8;
const int MEDIAPLAYER_BASS = wxID_HIGHEST + 9;
const int MEDIAPLAYER_MIDRANGE = wxID_HIGHEST + 10;
const int MEDIAPLAYER_TREBLE = wxID_HIGHEST + 11;
const int MEDIAPLAYER_BALANCE = wxID_HIGHEST + 12;
const int MEDIAPLAYER_NEXT_FOLDER = wxID_HIGHEST + 13;
const int MEDIAPLAYER_PREVIOUS_FOLDER = wxID_HIGHEST + 14;
const int MEDIAPLAYER_CURRENT_FOLDER = wxID_HIGHEST + 15;
const int MEDIAPLAYER_FILE_PLAY = wxID_HIGHEST + 16;
const int MEDIAPLAYER_SHUFFLE = wxID_HIGHEST + 17;
const int MEDIAPLAYER_REPEAT = wxID_HIGHEST + 18;
const int MEDIAPLAYER_LIST_ROOT = wxID_HIGHEST + 19;
const int MEDIAPLAYER_LIST_CURRENT = wxID_HIGHEST + 20;
const int MEDIAPLAYER_LIST_PARENT = wxID_HIGHEST + 21;
const int MEDIAPLAYER_LIST_CHILD = wxID_HIGHEST + 22;
const int MEDIAPLAYER_LIST_DEVICE = wxID_HIGHEST + 23;
const int MEDIAPLAYER_SAVE_PRESET = wxID_HIGHEST + 24;
const int MEDIAPLAYER_VOLUME = wxID_HIGHEST + 25;
const int MEDIAPLAYER_PROBE = wxID_HIGHEST + 26;
const int MEDIAPLAYER_ACK = wxID_HIGHEST + 27;
const int MEDIAPLAYER_CONFIRM = wxID_HIGHEST + 28;
const int MEDIAPLAYER_SELECT_PRESET = wxID_HIGHEST + 29;
const int MEDIAPLAYER_DELETE_PRESET = wxID_HIGHEST + 30;
const int MEDIAPLAYER_DELETE_ALLPRESET = wxID_HIGHEST + 31;
const int MEDIAPLAYER_SET_DEVICE_NAME = wxID_HIGHEST + 32;
const int MEDIAPLAYER_SET_ZONE_NAME = wxID_HIGHEST + 33;
const int MEDIAPLAYER_SET_TUNER_REGION = wxID_HIGHEST + 34;
const int MEDIAPLAYER_SET_TUNER_STEP = wxID_HIGHEST + 35;
const int MEDIAPLAYER_SET_VOLUME_LIMIT = wxID_HIGHEST + 36;
// Unused, playing with queues
const int MEDIAPLAYER_JOB_APPEND_TEXT = wxID_HIGHEST + 50;
const int MEDIAPLAYER_JOB_INCREMENT_COUNTER = wxID_HIGHEST + 51;

// These values are used by the NMEA 2000 messages to retrieve the folders.
// Cannot use absolute paths, it is a sequence of relative paths.
// Given Root/Music/Rolling Stones/Sticky Fingers and Root/Music/Beatles/Yellow Submarine
// And folder id's 0/0/1/0 and 0/0/33/0
// the sequence of directory commands would be 
// cd .. cd .. cd 33 cd 0 where cd .. is "Parent Folder"  and cd nn is "Child Folder"
enum FUSION_FOLDER_REQUEST {
	Root = 1,
	Child = 2,
	Parent = 3,
	Bookmark = 4, // Only seen on Zeus
	Device = 5
};

// From observation
enum FUSION_MEDIA_TYPE {
	type_am = 0,
	type_type_fm = 1,
	type_sirius = 2,
	type_aux = 3,
	type_ipod = 4,
	type_usb = 5,
	type_mtp = 9,
	type_bt = 10,
	type_dab = 14
};

enum FUSION_MEDIA_PORT {
	am = 0,
	fm = 1,
	sxm = 2,
	aux = 3,
	aux2 = 4,
	usb = 5,
	ipod = 6,
	mtp = 7,
	bt = 8,
	dab = 9,
	unknown = 99
};

enum FUSION_MEDIA_STATUS {
	not_installed = 0,
	not_present = 4,
	present = 5,
	connected = 15,
	paired = 25
};

struct FUSION_MEDIA_DEVICE {
	FUSION_MEDIA_PORT deviceId;
	FUSION_MEDIA_PORT sessionId;
	FUSION_MEDIA_TYPE deviceType;
	FUSION_MEDIA_STATUS status;
	wxString deviceName;
};

// Each Fusion Media Player has some characteristics
// Global, because also used by the preferences dialog
extern FUSION_MEDIA_PLAYER mediaPlayer;

// The Media Player Control plugin
class MediaPlayerPlugin : public opencpn_plugin_120, public wxEvtHandler {

public:
	// The constructor
	MediaPlayerPlugin(void *ppimgr);
	
	// and destructor
	~MediaPlayerPlugin(void);

	// Overridden OpenCPN plugin methods
	int Init(void) override;
	bool DeInit(void) override;
	int GetAPIVersionMajor() override;
	int GetAPIVersionMinor() override;
	int GetPlugInVersionMajor() override;
	int GetPlugInVersionMinor() override;
	wxString GetCommonName() override;
	wxString GetShortDescription() override;
	wxString GetLongDescription() override;
	wxBitmap *GetPlugInBitmap() override;
	int GetToolbarToolCount(void) override;
	void OnToolbarToolCallback(int id) override;
	void SetDefaults(void) override;
	void UpdateAuiStatus(void) override;
	void LateInit(void) override;
	void ShowPreferencesDialog(wxWindow *parent) override;
	void SetColorScheme(PI_ColorScheme cs) override;

	// Event Handler for events received from the Media Player dialog
	void OnDialogEvent(wxCommandEvent &event);

	// AUI Manager events
	void OnPaneClose(wxAuiManagerEvent& event);
			
private:
	// AUI Manager
	wxAuiManager *auiManager;

	// Reference to the OpenCPN configuration file
	wxFileConfig *configSettings;

	// Media Player Dialog 
	MediaPlayerDialog *mediaPlayerDialog;

	// Plugin bitmap
	wxBitmap pluginBitmap;

	// Reference to the OpenCPN window handle
	wxWindow *parentWindow;
	
	// Toolbar Id
	int mediaPlayerToolbar;

	// Toolbar State
	bool mediaPlayerDialogVisible;

	// Debugging
	bool debugSpew;

	// NMEA 2000
	std::string FindOutboundConnection(const std::string& connectionType);
	DriverHandle nmea2000Driver;

	// Fusion Media Player - Manufacturer Proprietary Fast Message
	void HandleN2K_130820(ObservedEvt ev);
	std::shared_ptr<ObservableListener> listener_130820;

	// Media Player stuff

	// Status of the media player
	bool isOn;
	bool isMuted;
	bool isPlaying;

	// Volume, balance and tone controls 
	int zone0Volume;
	int zone1Volume;
	unsigned char bass;
	unsigned char midrange;
	unsigned char treble;
	unsigned char balance;
	
	// Each media source is identified by a sessionId, aka a handle
	unsigned char sessionId;

	// When using a USB device, this uniquely identifies the remote control unit
	// Guessing that the player keeps a "cursor" for each remote control if they are 
	// browsing folders and files
	unsigned char headUnitId;

	// Used to determine if we have received all of the device names
	unsigned char totalSources;

	// AM/FM Radio Frequency and name (am is scaled by 1e-3, fm by 1e-6)
	unsigned int frequency;
	unsigned char lastPresetAM;
	unsigned char lastPresetFM;
	wxString radioStationName; // The RDS stuff for FM radios
	std::map<unsigned int, unsigned int> presetFM;
	std::map<unsigned int, unsigned int> presetAM;

	// Each music track has an album, artist, name, length and id
	wxString trackName;
	wxString albumName;
	wxString artistName;
	unsigned int trackLength;
	unsigned int trackId;
	// Not sure where this is used in the UI
	int totalTracks;

	// For Music Folders & Tracks, there's a sequence of requests/acks
	// to populate the file list
	unsigned int receivedRecords;

	// In case I ever want to use paging to retrieve items
	const unsigned int recordPageSize = 10;

	// Each folder/track has a type and id. Metadata used for the file list ui
	unsigned int folderId;
	wxString folderName;
	MEDIA_FILE_TYPE folderType;

	// The handshaking when navigating up and down folders
	// These functions are currently unused
	void SendConfirm(const int recordsReceived);
	void SendAck(void);
	
	// All of the devices/sources present on the media player. 
	// Populates the source selection etc.
	std::map<unsigned int, FUSION_MEDIA_DEVICE> mediaDevices;
	unsigned char LookupSourceId(const wxString& sourceName);
	wxString LookupSourceName(const unsigned char sourceId);
	void UpdateUI(const unsigned char session);
};

#endif 

