// Copyright(C) 2021-2026 by Steven Adler
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
// 2.0 Independant of TwoCan plugin, uses OpenCPN NMEA 2000 interfaces
// 
// BUG BUG To Do
// Pre-set radio stations, Bluetooth pairing with media devices.

#include "mediaplayer_plugin.h"

// BUG BUG REMOVE
#include <wx/socket.h>

// Some globals accessed by both the plugin and the dialog

// Mute/Unmute
bool isMuted = false;

// Power On/Off
bool isOn = false;

// Pause/Play
bool isPlaying = false;

// The name of the source input (from the radio boxes)
wxString mediaSource;

// BUG BUG REMOVE 
// Debug spew via UDP
wxDatagramSocket *debugSocket;
wxIPV4address addrLocal;
wxIPV4address addrPeer;

// The class factories, used to create and destroy instances of the PlugIn
extern "C" DECL_EXP opencpn_plugin* create_pi(void *ppimgr) {
	return new MediaPlayerPlugin(ppimgr);
}

extern "C" DECL_EXP void destroy_pi(opencpn_plugin* p) {
	delete p;
}

// Constructor
MediaPlayerPlugin::MediaPlayerPlugin(void *ppimgr) : opencpn_plugin_118(ppimgr), wxEvtHandler() {

	// Initialize Advanced User Interface Manager (AUI)
	auiManager = GetFrameAuiManager();

	// Generate a random number for the folder session id
	folderSessionId = rand() & 255;

	// Initialize Images
	wxString shareLocn = GetPluginDataDir(PLUGIN_PACKAGE_NAME) + 
		wxFileName::GetPathSeparator() + _T("data") + wxFileName::GetPathSeparator();

	pluginBitmap = GetBitmapFromSVGFile(shareLocn + "media-player-plugin.svg", 32, 32);
}

// Destructor
MediaPlayerPlugin::~MediaPlayerPlugin(void) {
	// Nothing to do...
}

int MediaPlayerPlugin::Init(void) {
	// Maintain a reference to the OpenCPN window to use as the parent
	parentWindow = GetOCPNCanvasWindow();

	// Maintain a reference to the OpenCPN configuration object 
	configSettings = GetOCPNConfigObject();

	// Load Configuration Setings
	// BUG BUG Add Presets for the Radio Stations ??
	// BUG BUG Balance & Tone Controls
	if (configSettings) {
		configSettings->SetPath(_T("/PlugIns/MediaPlayer"));
		configSettings->Read(_T("Source"), &mediaSource, "fm");
		configSettings->Read(_T("Zone0Volume"), &zone0Volume, 3);
		configSettings->Read(_T("Zone1Volume"), &zone1Volume, 3);
		configSettings->Read(_T("Balance"), &balance, 0);
		configSettings->Read(_T("Bass"), &bass, 3);
		configSettings->Read(_T("Midrange"), &midrange, 3);
		configSettings->Read(_T("Treble"), &treble, 3);
		configSettings->Read(_T("Visible"), &mediaPlayerDialogVisible, false);
	}
	else {
		mediaSource = "fm";
		zone0Volume = 3;
		zone1Volume = 3;
		balance = 0;
		bass = 3;
		midrange = 3;
		treble = 3;
		mediaPlayerDialogVisible = false;
	}


	// Load toolbar icons
	wxString shareLocn = GetPluginDataDir(PLUGIN_PACKAGE_NAME) + wxFileName::GetPathSeparator() + _T("data") + wxFileName::GetPathSeparator();
	
	wxString normalIcon = shareLocn + _T("mediaplayer_icon_normal.svg");
	wxString toggledIcon = shareLocn + _T("mediaplayer_icon_toggled.svg");
	wxString rolloverIcon = shareLocn + _T("mediaplayer_icon_rollover.svg");

	// Insert the toolbar icons
	mediaPlayerToolbar = InsertPlugInToolSVG(_T(""), normalIcon, rolloverIcon, toggledIcon, wxITEM_CHECK, _(PLUGIN_COMMON_NAME), _T(""), NULL, -1, 0, this);

	// Instantiate the media player dialog
	mediaPlayerDialog = new  MediaPlayerDialog(parentWindow, this);

	// Display the default/previously saved values
	mediaPlayerDialog->SetZone0Volume(zone0Volume);
	mediaPlayerDialog->SetZone1Volume(zone1Volume);
	mediaPlayerDialog->SetSource(mediaSource);

	// Wire up the event handler to receive events from the dialog
	Connect(wxEVT_MEDIAPLAYER_DIALOG_EVENT, wxCommandEventHandler(MediaPlayerPlugin::OnDialogEvent));
	
	// Initialise NMEA 2000 Listeners
	// PGN 126983 Alert
	wxDEFINE_EVENT(EVT_N2K_130820, ObservedEvt);
	NMEA2000Id id_130820 = NMEA2000Id(130820);
	listener_130820 = std::move(GetListener(id_130820, EVT_N2K_130820, this));
	Bind(EVT_N2K_130820, [&](ObservedEvt ev) { HandleN2K_130820(ev); });

	// BUG BUG REMOVE
	// Initialize UDP socket for debug spew
	addrLocal.Hostname();
	addrPeer.Hostname("127.0.0.1");
	addrPeer.Service(3002);

	debugSocket = new wxDatagramSocket(addrLocal, wxSOCKET_NONE);
	
	if (!debugSocket->IsOk()) {
		wxLogMessage(_T("ERROR: failed to create UDP peer socket"));
	}

	folderId = 0;

	// Notify OpenCPN what events we want to receive callbacks for
	return (WANTS_CONFIG | WANTS_TOOLBAR_CALLBACK | INSTALLS_TOOLBAR_TOOL | WANTS_PLUGIN_MESSAGING | USES_AUI_MANAGER | WANTS_LATE_INIT);
}

void MediaPlayerPlugin::LateInit(void) {
	// For some reason unbeknownst to me, the aui manager fails to wire up correctly if done
	// in the constructor or init. Seems to wire up correctly here though....

	// Load our dialog into the AUI Manager
	wxAuiPaneInfo paneInfo;
	paneInfo.Name(_T(PLUGIN_COMMON_NAME));
	paneInfo.Caption(PLUGIN_SHORT_DESCRIPTION);
	paneInfo.CloseButton(true);
	paneInfo.Float();
	paneInfo.Dockable(false);
	paneInfo.Show(mediaPlayerDialogVisible);
	auiManager->AddPane(mediaPlayerDialog, paneInfo);
	auiManager->Connect(wxEVT_AUI_PANE_CLOSE, wxAuiManagerEventHandler(MediaPlayerPlugin::OnPaneClose), NULL, this);
	auiManager->Update();

	// Handle the OnClose Event
	//paneInfo = auiManager->GetPane(_T("Fusion Media Player"));
	//if (paneInfo.IsOk()) {
		//auiManager->Connect(wxEVT_AUI_PANE_CLOSE, wxAuiManagerEventHandler(MediaPlayerPlugin::OnPaneClose), NULL, this);
	//}
}

// OpenCPN is either closing down, or we have been disabled from the Preferences Dialog
bool MediaPlayerPlugin::DeInit(void) {
	Disconnect(wxEVT_MEDIAPLAYER_DIALOG_EVENT, wxCommandEventHandler(MediaPlayerPlugin::OnDialogEvent));
	auiManager->Disconnect(wxEVT_AUI_PANE_CLOSE, wxAuiManagerEventHandler(MediaPlayerPlugin::OnPaneClose), NULL, this);
	auiManager->UnInit();
	auiManager->DetachPane(mediaPlayerDialog);
	delete mediaPlayerDialog;	

	if (configSettings) {
		configSettings->SetPath(_T("/PlugIns/MediaPlayer"));
		configSettings->Write(_T("Source"), mediaSource);
		configSettings->Write(_T("Zone0Volume"), zone0Volume);
		configSettings->Write(_T("Zone1Volume"), zone1Volume);
		configSettings->Write(_T("Balance"), balance);
		configSettings->Write(_T("Bass"), bass);
		configSettings->Write(_T("Midrange"), midrange);
		configSettings->Write(_T("Treble"), treble);
		configSettings->Write(_T("Visible"), mediaPlayerDialogVisible);
	}

	// BUG BUG DEBUG REMOVE
	debugSocket->Close();
	return true;
}

// Overridden OpenCPN methods

// Indicate what version of the OpenCPN Plugin API we support
int MediaPlayerPlugin::GetAPIVersionMajor() {
	return OCPN_API_VERSION_MAJOR;
}

int MediaPlayerPlugin::GetAPIVersionMinor() {
	return OCPN_API_VERSION_MINOR;
}

// The Media Player plugin version numbers. 
int MediaPlayerPlugin::GetPlugInVersionMajor() {
	return PLUGIN_VERSION_MAJOR;
}

int MediaPlayerPlugin::GetPlugInVersionMinor() {
	return PLUGIN_VERSION_MINOR;
}

// Descriptions for the Media Player plugin
wxString MediaPlayerPlugin::GetCommonName() {
	return _T(PLUGIN_COMMON_NAME);
}

wxString MediaPlayerPlugin::GetShortDescription() {
	return _T(PLUGIN_SHORT_DESCRIPTION);
}

wxString MediaPlayerPlugin::GetLongDescription() {
	return _T(PLUGIN_LONG_DESCRIPTION);
}

// Media Player plugin icon
wxBitmap* MediaPlayerPlugin::GetPlugInBitmap() {
	wxString pluginFolder = GetPluginDataDir(PLUGIN_PACKAGE_NAME) + wxFileName::GetPathSeparator() + _T("data") + wxFileName::GetPathSeparator();

	pluginBitmap = GetBitmapFromSVGFile(pluginFolder + _T("mediaplayer_icon_toggled.svg"), 32, 32);
	return &pluginBitmap;
}

// We install one toolbar items
int MediaPlayerPlugin::GetToolbarToolCount(void) {
 return 1;
}

int MediaPlayerPlugin::GetToolbarItemId() { 
	return mediaPlayerToolbar; 
}

void MediaPlayerPlugin::SetDefaults(void) {
	// Is called when the plugin is enabled
	// Anything to do ?
}

// UpdateAUI Status is invoked by OpenCPN when the saved AUI perspective is loaded
void MediaPlayerPlugin::UpdateAuiStatus(void) {
	auiManager->GetPane(_T(PLUGIN_COMMON_NAME)).Show(mediaPlayerDialogVisible);
	auiManager->Update();
	SetToolbarItemState(mediaPlayerToolbar, mediaPlayerDialogVisible);
}

// Toggle the display of dialog as appropriate when the toolbar button is pressed
void MediaPlayerPlugin::OnToolbarToolCallback(int id) {
	if (id == mediaPlayerToolbar) {
		mediaPlayerDialogVisible = !mediaPlayerDialogVisible;
		auiManager->GetPane(_T(PLUGIN_COMMON_NAME)).Show(mediaPlayerDialogVisible);
		auiManager->Update();
		SetToolbarItemState(mediaPlayerToolbar, mediaPlayerDialogVisible);
	}
}

// Keep the toolbar in synch with the pane state (user has closed the dialog from the "x" button
void MediaPlayerPlugin::OnPaneClose(wxAuiManagerEvent& event) {
	// BUG BUG Do I need to check that the event belongs to our pane/window ?
	wxAuiPaneInfo *paneInfo = event.GetPane();
	if (paneInfo->name == _T(PLUGIN_COMMON_NAME)) {
		mediaPlayerDialogVisible = false;
		SetToolbarItemState(mediaPlayerToolbar, mediaPlayerDialogVisible);
		event.Skip();
	}
}

// Receive the OpenCPN Plugin Messages. Only interested in message id "TWOCAN_MEDIA_RESPONSE"
void MediaPlayerPlugin::SetPluginMessage(wxString &message_id, wxString &message_body) {

	// Brief description of the JSON schema used by TwoCan
	// Device Name
	// root["entertainment"]["device"]["name"]

	// Input source
	// root["entertainment"]["device"]["input"]["name"]
	// root["entertainment"]["device"]["input"]["sourceid"]

	// Current session
	// root["entertainment"]["device"]["source"]["ssessionid"]

	// Volume
	// root["entertainment"]["device"]["zone0"]["volume"]
	// root["entertainment"]["device"]["zone1"]["volume"]
	// root["entertainment"]["device"]["zone2"]["volume"]

	// Tone Settings
	// root["entertainment"]["device"]["tone"]["bass"]
	// root["entertainment"]["device"]["tone"]["midrange"]
	// root["entertainment"]["device"]["tone"]["treble"]

	// Balance
	// root["entertainment"]["device"]["zone0"]["balance"]
	// root["entertainment"]["device"]["zone1"]["balance"]
	// root["entertainment"]["device"]["zone2]["balance"]

	// Radio
	// root["entertainment"]["device"]["radio"]["name"]
	// root["entertainment"]["device"]["radio"]["frequency"] 

	// Music Tracks
	// root["entertainment"]["device"]["track"]["name"]
	// root["entertainment"]["device"]["track"]["trackid"]
	// root["entertainment"]["device"]["track"]["tracks"]
	// root["entertainment"]["device"]["track"]["length"] 
	// root["entertainment"]["device"]["track"]["elapsedtime"]

	// Device Status
	// root["entertainment"]["device"]["power"]
	// root["entertainment"]["device"]["playing"]
	// root["entertainment"]["device"]["repeat"]
	// root["entertainment"]["device"]["shuffle"]
	// root["entertainment"]["device"]["pause"]
	// root["entertainment"]["device"]["next"]
	// root["entertainment"]["device"]["previous"]
	// root["entertainment"]["device"]["mute"]

	// Media
	// root["entertainment"]["device"]["media"]["folderid"]
	// root["entertainment"]["device"]["media"]["sessionid"]

	wxJSONReader jsonReader;
	wxJSONWriter jsonWriter;
	wxJSONValue root;

	if (message_id == _T("TWOCAN_MEDIA_RESPONSE")) {
		if (jsonReader.Parse(message_body, &root) > 0) {
			// Save the erroneous json text for debugging
			wxLogMessage("TwoCan Media Player, JSON Error in following text:");
			wxLogMessage("%s", message_body);
			wxArrayString jsonErrors = jsonReader.GetErrors();
			for (auto it : jsonErrors) {
				wxLogMessage(it);
			}
			return;
		}

		// BUG BUG DEBUG REMOVE
		//debugSocket->SendTo(addrPeer, message_body.data(), message_body.Length());

		OutputDebugString(L"Media Player\n");

		// Display the name of the audio device
		if (root["entertainment"]["device"].HasMember("name")) {
			auiManager->GetPane(_T(PLUGIN_COMMON_NAME)).caption = root["entertainment"]["device"]["name"].AsString();
			auiManager->Update();
		}

		// Display the power on/off state
		if (root["entertainment"]["device"].HasMember("power")) {
			bool powerState = root["entertainment"]["device"]["power"].AsBool();
			mediaPlayerDialog->SetPowerState(powerState);
		}

		if (root["entertainment"]["device"].HasMember("repeat")) {
			bool repeatState = root["entertainment"]["device"]["repeat"].AsBool();
			mediaPlayerDialog->SetRepeat(repeatState);
		}

		if (root["entertainment"]["device"].HasMember("shuffle")) {
			bool shuffleState = root["entertainment"]["device"]["shuffle"].AsBool();
			mediaPlayerDialog->SetShuffle(shuffleState);
		}

		// BUG BUG Check if used
		//if (root["entertainment"]["device"]["source"].HasMember("count")) {
		//	totalSources = root["entertainment"]["device"]["source"]["count"].AsInt();
		//}

		// Display the media source input
		if (root["entertainment"]["device"]["input"].HasMember("name")) {
			sessionId = root["entertainment"]["device"]["input"]["sourceid"].AsInt();
			mediaSource = root["entertainment"]["device"]["source"]["name"].AsString().Lower();
			mediaPlayerDialog->SetSource(mediaSource);
			// Clear the status & progress bar, until we receive new info
			mediaPlayerDialog->SetStatus(wxEmptyString);

			// if a usb device, retrieve the list of folders on the device
			if (mediaSource.Lower() == "usb") {
				root.Clear();
				root["entertainment"]["device"]["media"]["request"] = THIS_FOLDER;
				root["entertainment"]["device"]["media"]["folderid"] = 0;
				jsonWriter.Write(root, message_body);
				SendPluginMessage(_T("TWOCAN_MEDIA_REQUEST"), message_body);

				debugSocket->Write(message_body, message_body.Length());
			}
		}

		// Display the mute/unmute status
		if (root["entertainment"]["device"].HasMember("mute")) {
			mediaPlayerDialog->SetMuteState(root["entertainment"]["device"]["mute"].AsBool());
		}

		// Display the playing/paused state
		if (root["entertainment"]["device"].HasMember("playing")) {
			mediaPlayerDialog->SetPlayState(root["entertainment"]["device"]["playing"].AsBool());

		}

		// If playing media from a USB device, populate the progress bar
		// If an AM or FM radio, use the progress bar to display the tuned frequency
		if (root["entertainment"]["device"]["track"].HasMember("length")) {
			mediaPlayerDialog->SetGaugeRange(root["entertainment"]["device"]["track"]["length"].AsInt());
		}

		// Set the progress bar value
		if (root["entertainment"]["device"]["track"].HasMember("elapsedtime")) {
			mediaPlayerDialog->SetGaugeValue(root["entertainment"]["device"]["track"]["elapsedtime"].AsInt());

			debugSocket->SendTo(addrPeer, message_body.data(), message_body.Length());
		}

		// And display the track title
		if (root["entertainment"]["device"]["track"].HasMember("name")) {
			mediaPlayerDialog->SetStatus(root["entertainment"]["device"]["track"]["name"].AsString());

			debugSocket->SendTo(addrPeer, message_body.data(), message_body.Length());
		}

		// Similarly if a radio, display the station name (if available) and frequency
		if (root["entertainment"]["device"]["radio"].HasMember("name")) {
			wxString stationName = wxString::Format("%s (%0.2f)",
				root["entertainment"]["device"]["radio"]["name"].AsString(), frequency);

			mediaPlayerDialog->SetStatus(stationName);
		}

		// Save the radio frequency for tuning up and down and for station display
		if (root["entertainment"]["device"]["radio"].HasMember("frequency")) {
			frequency = root["entertainment"]["device"]["radio"]["frequency"].AsDouble();
		}

		// Set zone values
		if (root["entertainment"]["device"]["zone0"].HasMember("name")) {
			mediaPlayerDialog->SetZone0Name(root["entertainment"]["device"]["zone0"]["name"].AsString());
		}

		if (root["entertainment"]["device"]["zone0"].HasMember("volume")) {
			mediaPlayerDialog->SetZone0Volume(root["entertainment"]["device"]["zone0"]["volume"].AsInt());
		}
		// BUG BUG, My UI only has one balance control for all zones
		if (root["entertainment"]["device"]["zone0"].HasMember("balance")) {
			balance = (root["entertainment"]["device"]["zone0"]["balance"].AsInt());
			// Use two's complement to obtain +/- for left/right balance
			// BUG BUG, Possibly a bug in TwoCan Plugin, not using an unsigned char
			if ((balance & 0x80) == 0x80) {
				balance -= 256;
			}
			mediaPlayerDialog->SetBalance(balance);
		}

		if (root["entertainment"]["device"]["zone1"].HasMember("name")) {
			mediaPlayerDialog->SetZone1Name(root["entertainment"]["device"]["zone1"]["name"].AsString());
		}

		if (root["entertainment"]["device"]["zone1"].HasMember("volume")) {
			mediaPlayerDialog->SetZone1Volume(root["entertainment"]["device"]["zone1"]["volume"].AsInt());
		}

		// As above, the UI only has a single balance control
		/*
		if (root["entertainment"]["device"]["zone1"].HasMember("balance")) {
		balance = (root["entertainment"]["device"]["zone0"]["balance"].AsInt());
		if ((balance & 0x80) == 0x80) {
			balance -= 256;
		}
		mediaPlayerDialog->SetBalance(balance);
		}
		*/

		if (root["entertainment"]["device"]["tone"].HasMember("bass")) {
			mediaPlayerDialog->SetBass(root["entertainment"]["device"]["tone"]["bass"].AsInt());
		}
		
		if (root["entertainment"]["device"]["tone"].HasMember("midrange")) {
			mediaPlayerDialog->SetMidrange(root["entertainment"]["device"]["tone"]["midrange"].AsInt());
		}

		if (root["entertainment"]["device"]["tone"].HasMember("treble")) {
			mediaPlayerDialog->SetTreble(root["entertainment"]["device"]["tone"]["treble"].AsInt());
		}

		if (root["entertainment"]["device"].HasMember("shuffle")) {
			mediaPlayerDialog->SetShuffle(root["entertainment"]["device"]["shuffle"].AsBool());
		}

		if (root["entertainment"]["device"].HasMember("repeat")) {
			mediaPlayerDialog->SetRepeat(root["entertainment"]["device"]["repeat"].AsBool());
		}

		// A sequence of acks/confirms
		if (root["entertainment"]["device"]["media"].HasMember("rootfolder")) {

			debugSocket->SendTo(addrPeer, message_body.data(), message_body.Length());

			//folderSessionId = root["entertainment"]["device"]["media"]["foldersessionid"].AsInt();
			folderId = root["entertainment"]["device"]["media"]["folderid"].AsInt();

			// Send an ack
			root.Clear();
			root["entertainment"]["device"]["media"]["ack"] = true;
			jsonWriter.Write(root, message_body);
			SendPluginMessage(_T("TWOCAN_MEDIA_REQUEST"), message_body);

		}

		if (root["entertainment"]["device"]["media"].HasMember("count")) {

			expectedRecords = root["entertainment"]["device"]["media"]["count"].AsInt();
			receivedRecords = 0;

			//wxMessageBox(wxString::Format("Records: %d, Window: %d",
			//	expectedRecords, wxMin(expectedRecords, recordPageSize)));

			// Send a confirm
			root.Clear();
			root["entertainment"]["device"]["media"]["confirm"] = true;
			root["entertainment"]["device"]["media"]["folderid"] = folderId;
			root["entertainment"]["device"]["media"]["recordsreceived"] = expectedRecords > recordPageSize ? recordPageSize : expectedRecords;
			jsonWriter.Write(root, message_body);
			SendPluginMessage(_T("TWOCAN_MEDIA_REQUEST"), message_body);

		}

		// Page Returned Calcs
		//int pagesize = 10;
		//int received = 0;
		//int total = 43;
		//	for (int i = 0; i < total; i++) {
		//		received++;
		//		if ((received % pagesize) == 0) {
		//			printf("Received: %d\n", received);
		//			printf("Next page: %d\n", (total - received) > pagesize ? pagesize : total - received);
		//		}
		//}

		if (root["entertainment"]["device"]["media"].HasMember("foldername")) {

			receivedRecords++;
			
			if (receivedRecords == 1) {
				// This is the first entry, clear the list
				mediaPlayerDialog->ClearFileList();
			}

			wxString folderName = root["entertainment"]["device"]["media"]["foldername"].AsString();
			folderId = root["entertainment"]["device"]["media"]["folderid"].AsInt();
			int folderType = root["entertainment"]["device"]["media"]["foldertype"].AsInt();

			switch (folderType) {
				case MEDIA_FILE_TYPE::MTPFOLDER: // The top level device folder, aka root folder
					mediaPlayerDialog->SetRootFolder(folderName);
					break;
				case MEDIA_FILE_TYPE::DEVICEFOLDER: // physical or virtual folders
				case MEDIA_FILE_TYPE::PHYSICALFOLDER:
					mediaPlayerDialog->AppendFolder(folderName, (MEDIA_FILE_TYPE)folderType, folderId);
				
					break;
				case MEDIA_FILE_TYPE::MUSICTRACK: // self explanatory
					mediaPlayerDialog->AppendFile(folderName, folderId);
					break;
			}

			wxString debugString = (wxString::Format("Folder Name: %s\nRemainder: %d", 
				folderName, receivedRecords % recordPageSize));
			debugSocket->SendTo(addrPeer, debugString, debugString.Length());

			if (((receivedRecords % recordPageSize) == 0) && (receivedRecords < expectedRecords)) {
			//	
			//	wxMessageBox("Next Set of Records");

			// Send a confirm for the next sequence of records
				root.Clear();
				root["entertainment"]["device"]["media"]["confirm"] = true;
				root["entertainment"]["device"]["media"]["folderid"] = folderId;
				root["entertainment"]["device"]["media"]["recordsreceived"] = (expectedRecords - receivedRecords) > recordPageSize ? recordPageSize : expectedRecords - receivedRecords;
				jsonWriter.Write(root, message_body);
			//	SendPluginMessage(_T("TWOCAN_MEDIA_REQUEST"), message_body);

			}
			
		}
	}
	OutputDebugString(L"End Mediaplayer\n");
}

// Handle events from the dialog
// Encode the JSON commands to send to the twocan plugin so it can generate the NMEA 2000 messages
void MediaPlayerPlugin::OnDialogEvent(wxCommandEvent &event) {
	wxString message_body;
	wxJSONValue root;
	wxJSONWriter writer;
	
	switch (event.GetId()) {
	
		case MEDIAPLAYER_ON: 
			// (event.GetString() == "On") ? true : false;
			// Turn On
			break;

		case MEDIAPLAYER_MUTE:
			// (event.GetString() == "Mute") ? true : false;
			// Set Mute
			break;

		case MEDIAPLAYER_ZONE0: 
			zone0Volume = event.GetInt();
			// Set Volume for zone 0
			break;

		case MEDIAPLAYER_ZONE1:
			zone1Volume = event.GetInt();
			// Set Volume for zone 1
			break;

		case MEDIAPLAYER_BALANCE:
			balance = event.GetInt();
			// Set Balance
			break;

		case MEDIAPLAYER_BASS:
			bass = event.GetInt();
			// Set Bass
			break;

		case MEDIAPLAYER_MIDRANGE:
			midrange = event.GetInt();
			// Set midrange
			break;

		case MEDIAPLAYER_TREBLE:
			//treble = event.GetInt();
			// Set Treble
			break;

		case MEDIAPLAYER_SHUFFLE:
			// = event.GetInt();
			// Set Shuffle
			break;

		case MEDIAPLAYER_REPEAT:
			// event.GetInt();
			// Set repeat
			break;

		case MEDIAPLAYER_LIST_CURRENT:
			// Get items in this folder;
			break;

		case MEDIAPLAYER_LIST_CHILD:
			// Get Children of this folder
			break;

		case MEDIAPLAYER_LIST_PARENT:
			// Get Parent to this folder
			break;
		
		case MEDIAPLAYER_NEXT:
			// fm steps are 5Mhz, am are 9 KHz, howver it looks like it does this automagically
			if ((mediaSource == "am") || (mediaSource == "fm")) {
				root["entertainment"]["device"]["frequency"] = frequency;
			}
			// Get Next Frequency
			break;

		case MEDIAPLAYER_PAUSE:
			//event.GetString() == "Play" ? true : false;
			// Set Play or Pause
			break;

		case MEDIAPLAYER_PREVIOUS:
			
			//if ((mediaSource == "am") || (mediaSource == "fm")) {
			//	root["entertainment"]["device"]["frequency"] = frequency;
			// Get Previous Station or Track
			break;

		case MEDIAPLAYER_NEXT_FOLDER:
			//["folderid"] = event.GetInt();
			// Get Next Folder
			break;

		case MEDIAPLAYER_PREVIOUS_FOLDER:
			// ["folderid"] = event.GetInt();
			//Get Previous Folder
			break;
		
		case MEDIAPLAYER_SOURCE:
			//event.GetString();
			// if  (event.GetString().Lower() == "usb") {
			//	// if we've switched to a USB device, retrieve and display the media folders and music track names
			//	//request the folders
			//	// 0 just gets message 16, ?
			//	// 1 is get current folder,
			//	// 2 is next
			//	// 3 is previous
			// Get Source

			// need to empty the tree control
			mediaPlayerDialog->ClearFileList();
			
			// Clear everything let the setters do their job
			mediaPlayerDialog->SetGaugeValue(0);
			mediaPlayerDialog->SetStatus(wxEmptyString);

			break;

		case MEDIAPLAYER_FILE_PLAY:
			//event.GetString();
			//Play
			break;

		default:
			event.Skip();
	}
}