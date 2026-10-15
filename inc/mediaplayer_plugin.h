// Copyright(C) 2021 by Steven Adler
//
// This file is part of Media Player Control plugin for OpenCPN.
//
// Media Player Control plugin for OpenCPN is free software: you can 
// redistribute it and/or modify  it under the terms of the GNU General 
// Public License as published by the Free Software Foundation, either version 3 
// of the License, or (at your option) any later version.
//
// Media Player Control plugin for OpenCPN is distributed in the hope that 
// it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty 
// of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
// See the GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with the mediaPlayerControl plugin for OpenCPN. 
// If not, see <https://www.gnu.org/licenses/>.
//
// NMEA2000® is a registered trademark of the National Marine Electronics Association
// Fusion® is a registered trademark of Fusion


#ifndef MEDIAPLAYERCONTROL_PLUGIN_H
#define MEDIAPLAYERCONTROL_PLUGIN_H

// Pre compiled headers 
#include "wx/wxprec.h"

#ifndef WX_PRECOMP
      #include <wx/wx.h>
#endif

// wxWidgets requirements
// Advanced User Interface
#include "wx/aui/aui.h"
#include <wx/aui/framemanager.h>

// Configuration
#include <wx/fileconf.h>

// JSON
#include "json_defs.h"
#include "jsonval.h"
#include "jsonreader.h"
#include "jsonwriter.h"

// STL
#include <unordered_map>

// Defines version numbers, names etc. for this plugin
// This is automagically constructed via version.h.in from CMakeLists.txt
#include "version.h"

// OpenCPN include file
#include "ocpn_plugin.h"

// Media Player Control Dialog
#include "mediaplayer_dialog.h"

// Plugin receives events from the Media Player Control Window
const wxEventType wxEVT_MEDIAPLAYER_DIALOG_EVENT = wxNewEventType();
const int MEDIAPLAYER_ON = wxID_HIGHEST + 1;
const int MEDIAPLAYER_MUTE = wxID_HIGHEST + 2;
const int MEDIAPLAYER_NEXT = wxID_HIGHEST + 3;
const int MEDIAPLAYER_PREVIOUS = wxID_HIGHEST + 4;
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
const int MEDIAPLAYER_LIST_CURRENT = wxID_HIGHEST + 19;
const int MEDIAPLAYER_LIST_PARENT = wxID_HIGHEST + 20;
const int MEDIAPLAYER_LIST_CHILD = wxID_HIGHEST + 21;

// These values are used by the NMEA 2000 messages to retrieve the folders.
// Cannot use absolute paths, it is a sequence of relative paths.
// Given Root/Music/Rolling Stones/Sticky Fingers and Root/Music/Beatles/Yellow Submarine
// And folder id's 0/0/1/0 and 0/0/33/0
// the sequence of directory commands would be 
// cd .. cd .. cd 33 cd 0 where cd .. is "Parent Folder"  and cd nn is "Child Folder"
const int ROOT_FOLDER = 0;
const int THIS_FOLDER = 1;
const int NEXT_FOLDER = 2;
const int PREVIOUS_FOLDER = 3;
const int BOOKMARK_FOLDER = 4;

// The Media Player Control plugin
class MediaPlayerPlugin : public opencpn_plugin_118, public wxEvtHandler {

public:
	// The constructor
	MediaPlayerPlugin(void *ppimgr);
	
	// and destructor
	~MediaPlayerPlugin(void);

	// Overridden OpenCPN plugin methods
	int Init(void);
	bool DeInit(void);
	int GetAPIVersionMajor();
	int GetAPIVersionMinor();
	int GetPlugInVersionMajor();
	int GetPlugInVersionMinor();
	wxString GetCommonName();
	wxString GetShortDescription();
	wxString GetLongDescription();
	wxBitmap *GetPlugInBitmap();
	int GetToolbarToolCount(void);
	int GetToolbarItemId(void);
	void OnToolbarToolCallback(int id);
	void SetDefaults(void);
	void SetPluginMessage(wxString &message_id, wxString &message_body);
	void UpdateAuiStatus(void);
	void LateInit(void);
	
	// Event Handler for OpenCPN Plugin Messages
	void OnPluginEvent(wxCommandEvent &event);

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

	// Media Player stuff
	// Each Fusion Media Player has a name
	wxString deviceName;

	// Each music track has a name & length
	wxString trackName;
	int trackLength;

	int zone0Volume;
	int zone1Volume;
	
	int bass;
	int midrange;
	int treble;
	
	int balance;
	
	// Each media source is identified by a sessionId
	int sessionId;
	// When using a USB device, the folderSessioId is used to handle folder changes
	int folderSessionId;

	// BUG UNNECESSARY
	unsigned int totalSources;

	// AM/FM Radio Frequency
	double frequency;
	
	// For Music Folders & Tracks, there's a sequence of requests/acks
	// and a session Id that is reused for perusing the folders & tracks
	int receivedRecords;
	int expectedRecords;
	const int recordPageSize = 5;
	// Each folder & track has an Id. Used to construct relative paths
	int folderId;
	// When we connect to a USB device (MTP, IPod) this signals the request
	// to retrieve the folders & tracks
	bool rqstSourceChange;
	
	// BUG BUG Unneccessary  ??
	std::unordered_map<unsigned int, wxString> mediaSources;
};

#endif 

