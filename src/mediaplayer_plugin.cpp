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
// Fusion® is a registered trademark of Garmin Ltd.
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
// 01/10/2026 - 2.0 Independant of TwoCan plugin, uses native OpenCPN NMEA 2000 interfaces
// 
// BUG BUG To Do
// Bluetooth pairing with media devices.
// DAB Radio , Sirius XM Radio, DVD Player, Connected/Multi-Room, SubWoofer, 
// Auxilliary Amp. (I don't have access to any of these)

#include "mediaplayer_plugin.h"

// BUG BUG DEBUGGING
#include <wx/socket.h>

// BUG BUG DEBUG
// Debug spew via UDP
wxDatagramSocket *debugSocket;
wxIPV4address addrLocal;
wxIPV4address addrPeer;
wxString debugMessage;

// The class factories, used to create and destroy instances of the PlugIn
extern "C" DECL_EXP opencpn_plugin* create_pi(void *ppimgr) {
	return new MediaPlayerPlugin(ppimgr);
}

extern "C" DECL_EXP void destroy_pi(opencpn_plugin* p) {
	delete p;
}

// Constructor
MediaPlayerPlugin::MediaPlayerPlugin(void *ppimgr) : opencpn_plugin_120(ppimgr), wxEvtHandler() {

	// Initialize Advanced User Interface Manager (AUI)
	auiManager = GetFrameAuiManager();

	// Generate a random number for the head unit id.
	// Used to uniquely identify the client when browsing folders and files
	headUnitId = rand() & 255;

	// Initializes media sources
	totalSources = 0;

	// Initialize the folder id (used for navigating USB albums and tracks)
	folderId = 0;

	// Initialize the state of the device
	isOn = false;
	isMuted = false;
	isPlaying = false;

	// SessionId is the currently playing media source
	sessionId = wxNOT_FOUND;

	// Initialize Plugin Icon
	wxString dataFolder = GetPluginDataDir(PKG_NAME) +
		wxFileName::GetPathSeparator() + _T("data") + wxFileName::GetPathSeparator();

	pluginBitmap = GetBitmapFromSVGFile(dataFolder + "mediaplayer_icon.svg", 32, 32);
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

	// Load Configuration Settings
	if (configSettings) {
		configSettings->SetPath(_T("/PlugIns/MediaPlayer"));
		configSettings->Read(_T("Visible"), &mediaPlayerDialogVisible, false);
		configSettings->Read(_T("Debug"), &debugSpew, false);
	}
	else {
		mediaPlayerDialogVisible = false;
		debugSpew = false;
	}

	// Load toolbar icons
	wxString dataFolder = GetPluginDataDir(PKG_NAME) + 
		wxFileName::GetPathSeparator() + _T("data") + wxFileName::GetPathSeparator();
	
	wxString normalIcon = dataFolder + _T("mediaplayer_icon_normal.svg");
	wxString toggledIcon = dataFolder + _T("mediaplayer_icon_toggled.svg");
	wxString rolloverIcon = dataFolder + _T("mediaplayer_icon_rollover.svg");

	// Insert the toolbar icons
	mediaPlayerToolbar = InsertPlugInToolSVG(_(PLUGIN_API_NAME), normalIcon, rolloverIcon, toggledIcon, wxITEM_CHECK, _(PLUGIN_API_NAME), wxString(PKG_DESCRIPTION), NULL, -1, 0, this);

	// Instantiate the media player dialog
	mediaPlayerDialog = new  MediaPlayerDialog(parentWindow, this);
	mediaPlayerDialog->SetPowerState(isOn);

	// Wire up the event handler to receive events from the dialog
	Connect(wxEVT_MEDIAPLAYER_DIALOG_EVENT, wxCommandEventHandler(MediaPlayerPlugin::OnDialogEvent));
	
	// Find an outbound NMEA 2000 connection to use with WriteCommDriverN2K
	nmea2000Driver = FindOutboundConnection("nmea2000");
	if (!nmea2000Driver.empty()) {
		wxLogMessage("Media Player Plugin, Using outbound NMEA 2000 network connection: %s", nmea2000Driver);
		// Plugins need to register what NMEA 2000 PGN's they transmit.
		// Fusion uses manufacturer prorietary PGN 126720 to control their media players
		std::vector<int> transmittedPGN = { 126720 };
		RegisterTXPGNs(nmea2000Driver, transmittedPGN);
	}

	// Initialise NMEA 2000 Listeners
	// PGN 130820 Manufacturer Proprietary Fast Message
	wxDEFINE_EVENT(EVT_N2K_130820, ObservedEvt);
	NMEA2000Id id_130820 = NMEA2000Id(130820);
	listener_130820 = std::move(GetListener(id_130820, EVT_N2K_130820, this));
	Bind(EVT_N2K_130820, [&](ObservedEvt ev) { HandleN2K_130820(ev); });

	// BUG BUG DEBUG
	// Initialize UDP socket for debug spew
	if (debugSpew) {
		addrLocal.Hostname();
		addrPeer.Hostname("127.0.0.1");
		addrPeer.Service(3002);

		debugSocket = new wxDatagramSocket(addrLocal, wxSOCKET_NONE);
	
		if (!debugSocket->IsOk()) {
			wxLogMessage(_T("Media Player Plugin, ERROR: failed to create UDP debugging socket"));
		}
	}

	// Notify OpenCPN what events we want to receive callbacks for
	return (WANTS_CONFIG | WANTS_TOOLBAR_CALLBACK | INSTALLS_TOOLBAR_TOOL | 
		WANTS_PREFERENCES | USES_AUI_MANAGER | WANTS_LATE_INIT );
}

void MediaPlayerPlugin::LateInit(void) {
	// For some reason unbeknownst to me, the aui manager fails to wire up correctly if done
	// in the constructor or init. Seems to wire up correctly here though....

	// Load our dialog into the AUI Manager
	wxAuiPaneInfo paneInfo;
	paneInfo.Name(PLUGIN_API_NAME);
	paneInfo.Caption(PLUGIN_API_NAME).CaptionVisible(true);
	paneInfo.CloseButton(true);
	paneInfo.Float();
	paneInfo.Dockable(false);
	paneInfo.Show(mediaPlayerDialogVisible);
	auiManager->AddPane(mediaPlayerDialog, paneInfo);
	auiManager->Connect(wxEVT_AUI_PANE_CLOSE, wxAuiManagerEventHandler(MediaPlayerPlugin::OnPaneClose), NULL, this);
	auiManager->Update();
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
		configSettings->Write(_T("Visible"), mediaPlayerDialogVisible);
	}

	// BUG BUG Should have an option to switch off the media player 
	// if OpenCPN is closing down.

	// BUG BUG DEBUG
	if (debugSpew) {
		debugSocket->Close();
	}

	return true;
}

// Overridden OpenCPN methods

// Indicate what version of the OpenCPN Plugin API we support
int MediaPlayerPlugin::GetAPIVersionMajor() {
	return atoi(API_VERSION);
}

int MediaPlayerPlugin::GetAPIVersionMinor() {
	std::string v(API_VERSION);
	size_t dotpos = v.find('.');
	return atoi(v.substr(dotpos + 1).c_str());
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
	return PLUGIN_API_NAME;
}

wxString MediaPlayerPlugin::GetShortDescription() {
	return PKG_SUMMARY;
}

wxString MediaPlayerPlugin::GetLongDescription() {
	return PKG_DESCRIPTION;
}

// Media Player plugin icon
wxBitmap* MediaPlayerPlugin::GetPlugInBitmap() {
	return &pluginBitmap;
}

// We install one toolbar item
int MediaPlayerPlugin::GetToolbarToolCount(void) {
 return 1;
}

void MediaPlayerPlugin::SetDefaults(void) {
	// Is called when the plugin is enabled
	// Anything to do ?
}

// Configure a few settings on the Fusion Media Player, device & zone names, etc.
void MediaPlayerPlugin::ShowPreferencesDialog(wxWindow* parent) {
	if (!isOn) {
		wxMessageBox("It appears that the Fusion Media Player is switched off", PLUGIN_API_NAME, wxICON_INFORMATION | wxOK);
	}
	else {
		MediaPlayerOptions *mediaPlayerOptions = new MediaPlayerOptions(parent, wxID_ANY, "Settings", parent->FromDIP(wxSize(500, 400)), this);
		if (wxID_OK == mediaPlayerOptions->ShowModal()) {
			// Nothing to do here. The queue of changes is dequeued in the dialog
			// And none of these are stored within OpenCPN configuration settings
		}
	}

}

// UpdateAUI Status is invoked by OpenCPN when the saved AUI perspective is loaded
void MediaPlayerPlugin::UpdateAuiStatus(void) {
	auiManager->GetPane(PLUGIN_API_NAME).Show(mediaPlayerDialogVisible);
	auiManager->Update();
	SetToolbarItemState(mediaPlayerToolbar, mediaPlayerDialogVisible);
}

// Toggle the display of dialog as appropriate when the toolbar button is pressed
void MediaPlayerPlugin::OnToolbarToolCallback(int id) {
	if (id == mediaPlayerToolbar) {
		mediaPlayerDialogVisible = !mediaPlayerDialogVisible;
		auiManager->GetPane(PLUGIN_API_NAME).Show(mediaPlayerDialogVisible);
		auiManager->Update();
		SetToolbarItemState(mediaPlayerToolbar, mediaPlayerDialogVisible);
	}
}

// Keep the toolbar in synch with the pane state (user has closed the dialog from the "x" button
void MediaPlayerPlugin::OnPaneClose(wxAuiManagerEvent& event) {
	// BUG BUG Do I need to check that the event belongs to our pane/window ?
	wxAuiPaneInfo *paneInfo = event.GetPane();
	if (paneInfo->name == PLUGIN_API_NAME) {
		mediaPlayerDialogVisible = false;
		SetToolbarItemState(mediaPlayerToolbar, mediaPlayerDialogVisible);
		event.Skip();
	}
}

// Support Day/Dusk/Night modes
void MediaPlayerPlugin::SetColorScheme(PI_ColorScheme cs) {
	mediaPlayerDialog->SetColourScheme(cs);

	// Could update AUI, but it seems to have no effect ?
	/*
	AppTheme theme = AppTheme::GetTheme(cs);
	wxAuiDockArt* art = auiManager->GetArtProvider();
	art->SetColour(wxAUI_DOCKART_BACKGROUND_COLOUR, theme.background);
	art->SetColour(wxAUI_DOCKART_ACTIVE_CAPTION_COLOUR, theme.background);
	art->SetColour(wxAUI_DOCKART_ACTIVE_CAPTION_TEXT_COLOUR,theme.foreground);
	art->SetColour(wxAUI_DOCKART_INACTIVE_CAPTION_COLOUR,theme.background);
	art->SetColour(wxAUI_DOCKART_INACTIVE_CAPTION_TEXT_COLOUR,theme.foreground);
	auiManager->Update();
	*/
}

// Helper function to find required driver handles
std::string MediaPlayerPlugin::FindOutboundConnection(const std::string& connectionType) {
	// Iterate through all of the OpenCPN connections
	for (const auto& driver : GetActiveDrivers()) {
		const auto& attributes = GetAttributes(driver);

		auto protocolIterator = attributes.find("protocol");
		auto directionIterator = attributes.find("ioDirection");

		if (protocolIterator != attributes.end() && directionIterator != attributes.end()) {
			// Found a connection matching the required connection and io direction
			if ((connectionType == protocolIterator->second) && ((directionIterator->second == "OUT")
				|| (directionIterator->second == "IN/OUT"))) {
				return driver;
			}
		}
	}
	return {};
}

// Parse Fusion Media Player responses
void MediaPlayerPlugin::HandleN2K_130820(ObservedEvt ev) {
	NMEA2000Id id_130820(130820);
	std::vector<unsigned char> payload = GetN2000Payload(id_130820, ev);

	// Remove the extraneous Actisense crap from the payload
	payload.erase(payload.begin(), payload.begin() + 13);

	// Check that it is indeed from a Fusion Media Player
	unsigned int manufacturerId;
	manufacturerId = payload[0] | ((payload[1] & 0x07) << 8);

	unsigned char industryCode;
	industryCode = (payload[1 + 1] & 0xE0) >> 5;

	if ((manufacturerId != 419) && (industryCode != 4)) {
		return;
	}

	// BUG BUG Perhaps these are an unsigned short (2 bytes) instead of two individual bytes ?
	unsigned char messageId;
	messageId = payload[2];

	unsigned char proprietaryId;
	proprietaryId = payload[3];
	
	if (proprietaryId != 0x80) {
		return;
	}

	// If we're receiving these messages, the media player was already on.
	if (!isOn) {
		wxCommandEvent* commandEvent = new wxCommandEvent(wxEVT_MEDIAPLAYER_DIALOG_EVENT, MEDIAPLAYER_ON);
		commandEvent->SetString(_T("On"));
		wxQueueEvent(this, commandEvent);
	}

	switch (messageId) {
	case 1: { 
		// Software Version
		// A3 99 01 80 01 00 19 00 02 00 C0 0D
		int playerMajorVersion = payload[4];
		int playerMinorVersion = payload[5];
		int playerPatchVersion = payload[6] | (payload[7] << 8);

		int linkMajorVersion = payload[8];
		int linkMinorVersion = payload[9];
		int linkPatchVersion = payload[10] | (payload[11] << 8);

		mediaPlayer.playerVersion = wxString::Format("%d.%d.%d",playerMajorVersion,
			playerMinorVersion, playerPatchVersion);
		mediaPlayer.linkVersion = wxString::Format("%d.%d.%d", linkMajorVersion,
			linkMinorVersion, linkPatchVersion);

		// BUG BUG DEBUG
		if (debugSpew) {
			debugMessage = wxString::Format("Id: %d. Ver: %d.%d.%d, Ver: %d.%d.%d",
				messageId, playerMajorVersion, playerMinorVersion, 
				playerPatchVersion, linkMajorVersion, linkMinorVersion, linkPatchVersion);
			debugSocket->SendTo(addrPeer, debugMessage, debugMessage.Length());
		}
		
		break;
	}

	case 2: { 
		// Media sources
		// A3 99 02 80 08 01 0A 25 02 42 54 00
		// A3 99 02 80 05 05 05 05 03 55 53 42 00
		// A3 99 02 80 03 03 02 05 03 41 75 78 00
		
		FUSION_MEDIA_DEVICE mediaDevice;
		mediaDevice.deviceId = (FUSION_MEDIA_PORT)payload[4];
		mediaDevice.sessionId = (FUSION_MEDIA_PORT)payload[5];
		mediaDevice.deviceType = (FUSION_MEDIA_TYPE)payload[6];
		mediaDevice.status = (FUSION_MEDIA_STATUS)payload[7];
		
		// Byte 8 is a length field
		// Bytes 9.. are a null terminated string, the length given by the length field
		mediaDevice.deviceName.clear();
		for (size_t i = 0; i < payload[8]; i++) {
			mediaDevice.deviceName.append(1, payload[9 + i]);
		}

		mediaDevices.emplace(mediaDevice.deviceId, mediaDevice);
		
		// Update the UI
		if (mediaDevices.size() == totalSources) {
			mediaPlayerDialog->ClearSource();
			for (auto it : mediaDevices) {
				if ((it.second.status & FUSION_MEDIA_STATUS::present) == FUSION_MEDIA_STATUS::present) {
					mediaPlayerDialog->AddSource(it.second.deviceName);
					if (it.second.deviceId == it.second.sessionId) {
						// This ensures the wxChoice control displays the correct device
						mediaPlayerDialog->SetSource(it.second.deviceName);
					}
				}
			}
		}

		// We've changed sources.
		if (sessionId != mediaDevice.sessionId) {
			sessionId = mediaDevice.sessionId;
			UpdateUI(sessionId);
		}

		// BUG BUG DEBUG
		if (debugSpew) {
			debugMessage = wxString::Format("Id: %d. Device: id: %d, name: %s, status: %d, session: %d",
				messageId, mediaDevice.deviceId, mediaDevice.deviceName, mediaDevice.status, mediaDevice.sessionId);
			debugSocket->SendTo(addrPeer, debugMessage.data(), debugMessage.Length());
		}

		break;
	}

	case 3: {
		// Total number of media sources
		// A3 99 03 80 0A
		totalSources = payload[4];
		mediaDevices.clear();

		break;
	}

	case 4: { 
		// Track Information
		// A3 99 04 80 05 01 00 01 00 00 00 0C 00 00 00 98 99 05 00 00 00 00 00
		
		unsigned char trackStatus = payload[5];

		// Device is off ??? we shouldn't be receiving these!!!
		if (trackStatus == 0x00) {
			
		}
		// Device is on
		else {
			// Playing
			if ((trackStatus & 0x01) == 0x01) {
				mediaPlayerDialog->SetPlayState(true);
			}
			// Paused
			if ((trackStatus & 0x02) == 0x02) {
				mediaPlayerDialog->SetPlayState(false);
			}
			// Repeat is enabled
			if ((trackStatus & 0x20) == 0x20) {
				// Need check boxes to display repeat and shuffle status
			}
			// Repeat is disabled
			else {
				
			}
			// Shuffle is enabled
			if ((trackStatus & 0x40) == 0x40) {
				
			}
			// Shuffle is disabled
			else {
				
			}

			// unsure about payload[6], Perhaps it is a ushort

			trackId = payload[7] | (payload[8] << 8) | (payload[9] << 16) | (payload[10] << 24);
			totalTracks = payload[11] | (payload[12] << 8) | (payload[13] << 16) | (payload[14] << 24);
			trackLength = 1e-3 * (payload[15] | (payload[16] << 8) | (payload[17] << 16) | (payload[18] << 24));

			mediaPlayerDialog->SetGaugeRange(trackLength);
		}

		// BUG BUG DEBUG
		if (debugSpew) {
			debugMessage = wxString::Format("Id: %d. Session  Id %d, Track Id: %d, Track Length: %d, Total Tracks: %d\n",
				messageId, payload[4], trackId, trackLength, totalTracks);
			debugSocket->SendTo(addrPeer, debugMessage, debugMessage.Length());
		}
		break;
	}

	case 5: { 
		// Track Name
		// A3 99 05 80 05 02 00 00 00 0C 30 32 5F 4A 75 64 67 65 6D 65 6E 74 00

		// When paused on BT, Track name is a space, track number is data unavailable
		// A3 99 05 80 08 FF FF FF FF 01 20 00 
		
		if (trackId == (payload[5] | (payload[6] << 8) | (payload[7] << 16) | (payload[8] << 24))) {
			trackName.Clear();
			for (size_t i = 0; i < payload[9]; i++) {
				trackName.append(1, payload[10 + i]);
			}
			// To show elapsed time
			// auto dv = std::div(elapsedTime, 60);
			// Do something with dv.quot, dv.rem and trackTime
			// Perhaps Display 2:25/5:45
			mediaPlayerDialog->SetStatus(wxString::Format("Now playing %s",trackName));
		}
		// BUG BUG DEBUG
		if (debugSpew) {
			debugMessage = wxString::Format("Id: %d. Session  Id %d, Track Id: %d, Track Name: %s\n",
				messageId, payload[4], trackId, trackName);
			debugSocket->SendTo(addrPeer, debugMessage, debugMessage.Length());
		}
		break;
	}

	case 6: { 
		// Album Artist
		// A3 99 06 80 05 01 00 00 00 0D 53 65 61 73 69 63 6B 20 53 74 65 76 65 00 5A
				
		if (trackId == (payload[5] | (payload[6] << 8) | (payload[7] << 16) | (payload[8] << 24))) {
			artistName.clear();
			for (size_t i = 0; i < payload[9]; i++) {
				artistName.append(1, payload[10 + i]);
			}

			// BUG BUG DEBUG
			if (debugSpew) {
				debugMessage = wxString::Format("Id: %d. Session  Id %d, Track Id: %d, Artist Name: %s\n", 
					messageId, payload[4], trackId, artistName);
				debugSocket->SendTo(addrPeer, debugMessage, debugMessage.Length());
			}
		}
		break;
	}

	case 7: { 
		// Album Title
		// A3 99 06 80 05 01 00 00 00 0D 53 65 61 73 69 63 6B 20 53 74 65 76 65 00 5A

		if (trackId == (payload[5] | (payload[6] << 8) | (payload[7] << 16) | (payload[8] << 24))) {
			albumName.clear();
			for (size_t i = 0; i < payload[9]; i++) {
				albumName.append(1, payload[10 + i]);
			}

			// BUG BUG DEBUG
			if (debugSpew) {
				debugMessage = wxString::Format("Id: %d. Session  Id %d, Track Id: %d, Album Name: %s\n",
					messageId, payload[4], trackId, albumName);
				debugSocket->SendTo(addrPeer, debugMessage, debugMessage.Length());
			}
		}
		break;
	}

	case 9: { 
		// Elapsed time
		// A3 99 09 80 05 53 CC 02 00
		int elapsedTime;
		elapsedTime = payload[5] | (payload[6] << 8) | (payload[7] << 16) | (payload[8] << 24);
		
		if (payload[4] == FUSION_MEDIA_PORT::bt) {
			mediaPlayerDialog->SetGaugeValue(0);
			mediaPlayerDialog->SetStatus("Now playing from Bluetooth");
		}
		else {
			elapsedTime *= 1e-3;
			mediaPlayerDialog->SetGaugeValue(elapsedTime);
		}

		// BUG BUG DEBUG
		if (debugSpew) {
			debugMessage = wxString::Format("Id: %d. Session  Id %d, Track Id: %d, Elapsed Time: %d\n",
				messageId, payload[4], trackId, elapsedTime);
			debugSocket->SendTo(addrPeer, debugMessage, debugMessage.Length());
		}
		break;
	}

	case 11: { 
		// Radio Station Frequency & Name
		// A3 99 0B 80 01 02 20 9C 52 05 BE 00 00

		if (sessionId != (FUSION_MEDIA_PORT)payload[4]) {
			sessionId = (FUSION_MEDIA_PORT)payload[4];
		}
		
		frequency = (payload[6] | (payload[7] << 8) | (payload[8] << 16) | (payload[9] << 24)); // * (sessionId == FUSION_MEDIA_PORT::am ? 1e-3 : 1e-6);

		// payload 10 perhaps signal quality

		radioStationName.Clear();
		for (size_t i = 0; i < payload[11]; i++) {
			radioStationName.append(1, payload[12 + i]);
		}

		if (sessionId == FUSION_MEDIA_PORT::am) {
		// AM Dial range 540 - 1600
			mediaPlayerDialog->SetGaugeRange(1100);
			mediaPlayerDialog->SetStatus(wxString::Format("AM %0.1f", frequency * 1e-3));
			mediaPlayerDialog->SetGaugeValue(round((frequency * 1e-3) - 520));
		}

		else if (sessionId == FUSION_MEDIA_PORT::fm) {
		// FM Dial range 87.5 - 110
			mediaPlayerDialog->SetGaugeRange(22);
			mediaPlayerDialog->SetStatus(wxString::Format("FM %s %0.1f", radioStationName, frequency * 1e-6));
			mediaPlayerDialog->SetGaugeValue(round((frequency * 1e-6) - 87));
		}

		// BUG BUG DEBUG
		if (debugSpew) {
			debugMessage = wxString::Format("Id: %d. Radio: Session Id: %d, Station Name: %s, Frequency: %d", 
				messageId, sessionId, radioStationName, frequency);
			debugSocket->SendTo(addrPeer, debugMessage, debugMessage.Length());
		}

		break;
	}

	case 14: {
		// A Preset Radio Station has been added
		// A3 99 14 80 1F 00 00 00 60 98 3D 06
		
		// BUG BUG Currently don't update the UI if a preset is deleted by another remote
		// Use a unordered map, to prevent duplicates and to persist both frequency and index
		// Assuming 15 (0x0F) presets per band.
		unsigned int newEntry;
		newEntry = payload[4] | (payload[5] << 8) | (payload[6] << 16) | (payload[7] << 24);
		unsigned int newFrequency;
		newFrequency = payload[8] | (payload[9] << 8) | (payload[10] << 16) | (payload[11] << 24);

		if ((newEntry > 0x1C) || (newEntry < 0x25)) {
			presetFM.emplace(newEntry, newFrequency);
		}
		if ((newEntry > 0x0C) || (newEntry < 0x15)) {
			presetAM.emplace(newEntry, newFrequency);
		}
		// BUG BUG What is the range for DAB ??
		
		// Update the UI
		if (sessionId == FUSION_MEDIA_PORT::am) {
			mediaPlayerDialog->ClearFileList();
			for (auto it : presetAM) {
				mediaPlayerDialog->AppendRadioStation(wxString::Format("%.1f KHz",it.second * 1e-3), it.second, it.first);
			}
		}
		if (sessionId == FUSION_MEDIA_PORT::fm) {
			mediaPlayerDialog->ClearFileList();
			for (auto it : presetFM) {
				mediaPlayerDialog->AppendRadioStation(wxString::Format("%.1f MHz", it.second * 1e-6), it.second, it.first);
			}
		}
		break;
	}

	case 15: { 
		// Selected Directory item
		// A3 99 0F 80 05 00 00 00 00 02 06
		// A3 99 0F 80 05 00 00 00 00 02 06

		// Ensure it is us controlling the media player
		// BUG BUG Remind myself why do I care about the session Id ??
		if ((sessionId == payload[4]) && (headUnitId == payload[10])) {
			folderId = payload[5] | (payload[6] << 8) | (payload[7] << 16) | (payload[8] << 24);
			
			// Note We don't need to act on the item type here
			// All handled when we receive the name of the item
			folderType = (MEDIA_FILE_TYPE)payload[9];

			// Send our acknowledgement
			wxCommandEvent* commandEvent = new wxCommandEvent(wxEVT_MEDIAPLAYER_DIALOG_EVENT, MEDIAPLAYER_ACK);
			wxQueueEvent(this, commandEvent);
			
			// BUG BUG DEBUG
			if (debugSpew) {
				debugMessage = wxString::Format("Id: %d. Folder; FolderId: %d, FolderType: %d",
					messageId, folderId, folderType);
				debugSocket->SendTo(addrPeer, debugMessage, debugMessage.Length());
			}

		}
		break;
	}

	case 16: { 
		// Number of items in folder
		// A3 99 10 80 05 03 00 00 00 06 

		if ((sessionId == payload[4]) && (headUnitId == payload[9])) {
			int folderCount = payload[5] | (payload[6] << 8) | (payload[7] << 16) | (payload[8] << 24);

			// Pause a little before sending the confirm
			wxThread::Sleep(50);

			// Confirm we are ready to receive (all of the items)
			// BUG BUG, if it was a large number consider paging.
			// Something along the lines of 
			// RecordsToReceive = blah blah
			// RecordsReceived += blah blah
			// StartRecord = 0;
			// Records to Fetch = std::min(10, RecordsToReceive - RecordsReceived)
			// If RecordsReceived == RecordsToReceive
			wxCommandEvent* commandEvent = new wxCommandEvent(wxEVT_MEDIAPLAYER_DIALOG_EVENT, MEDIAPLAYER_CONFIRM);
			commandEvent->SetInt(folderCount);
			wxQueueEvent(this, commandEvent);

			// BUG BUG DEBUG
			if (debugSpew) {
				debugMessage = wxString::Format("Id: %d. Count: Folder Items: %d, Head Unit Id: %d",
					messageId, folderCount, headUnitId);
				debugSocket->SendTo(addrPeer, debugMessage.data(), debugMessage.Length());
			}

		}
		
		break;
	}

	case 17: { 
		// file/folder Names
		// A3 99 11 80 05 00 00 00 00 4F 06 05 4D 55 53 49 43 00
		// A3 99 11 80 05 00 00 00 00 47 06 06 49 41 55 44 49 4F 00
		// A3 99 11 80 05 02 00 00 00 4F 06 06 53 59 53 54 45 4D 00

		if ((sessionId == payload[4]) && (headUnitId == payload[10])) {

			folderId = (payload[5] | (payload[6] << 8) | (payload[7] << 16) | (payload[8] << 24));

			folderType = (MEDIA_FILE_TYPE)(payload[9]);

			folderName.clear();
			for (int i = 0; i < payload[11]; i++) {
				folderName.append(1, payload[12 + i]);
			}

			// Update the UI
			if (folderType == MEDIA_FILE_TYPE::MTPFOLDER) { // Root Folder
				mediaPlayerDialog->SetRootFolder(folderName);
			}

			else if (folderType == MEDIA_FILE_TYPE::PHYSICALFOLDER) { // Normal folder
				mediaPlayerDialog->AppendFolder(folderName, MEDIA_FILE_TYPE::PHYSICALFOLDER, folderId);
			}

			// BUG BUG Treat these as needing a dummy roor
			else if (folderType == MEDIA_FILE_TYPE::VIRTUALFOLDER) { // Seem to be MTP folders
				mediaPlayerDialog->AppendFolder(folderName, MEDIA_FILE_TYPE::PHYSICALFOLDER, folderId);
			}

			else if (folderType == MEDIA_FILE_TYPE::DEVICEFOLDER) { // The shared storage on an Android ?
				mediaPlayerDialog->AppendFolder(folderName, MEDIA_FILE_TYPE::PHYSICALFOLDER, folderId);
			}

			else if (folderType == MEDIA_FILE_TYPE::MUSICTRACK || 
				(folderType == MEDIA_FILE_TYPE::MTPTRACK)) { 
				mediaPlayerDialog->AppendFile(folderName, folderId);
			}

			// BUG BUG DEBUG
			if (debugSpew) {
				debugMessage = wxString::Format("Id: %d. Names: Folder Id: %d, Folder Type: %d, Folder Name: %s", 
					messageId, folderId, folderType, folderName);
				debugSocket->SendTo(addrPeer, debugMessage.data(), debugMessage.Length());
			}
		}
		break;
	}

	case 19: { 
		// Auxilliary Media Source Gain
		// A3 99 13 80 04 00
		
		// No UI to display this

		// BUG BUG DEBUG
		if (debugSpew) {
			debugMessage = wxString::Format("Id: %d. Aux Gain: %d %d", 
				messageId, payload[4], payload[5]);
			debugSocket->SendTo(addrPeer, debugMessage, debugMessage.Length());
		}

		break;
	}

	case 20: {
		// Response to Tuner Setting changes. Presets, tuning steps, tuner region and others.
		// A3 99 14 80 00 00 00 00 03 00 00 00

		int index = payload[4] | (payload[5] << 8) | (payload[6] << 16) | (payload[7] << 24);
			
		int value = payload[8] | (payload[9] << 8) | (payload[10] << 16) | (payload[11] << 24);

		if (index == 0x00) {
			mediaPlayer.tunerSteps = value;
		}
		if (index == 0x07) {
			mediaPlayer.tunerRegion = value;
		}
		// AM Preset deleted
		if (index >= 0x0E && index <= 0x1C) {
			if (value == 0) {
				presetAM.erase(index);
			}
			// Update the UI
		}
		// FM preset deleted
		else if (index >= 0x1D && index <= 0x2C) {
			if (value == 0) {
				presetFM.erase(index);
			}
			// Update the UI
		}

		// BUG BUG DEBUG
		if (debugSpew) {
			debugMessage = wxString::Format("Id: %d. Setting: %d Value: %d",
				messageId, index, value);
			debugSocket->SendTo(addrPeer, debugMessage, debugMessage.Length());
		}

		break;
	}

	case 21: {
		// Tuner Settings. Presets, tuning steps, tuner region and others.
		// Hope I've got the ranges right ??
		// 0x0E - 0x1C = AM
		// 0x1D - 0x2C = FM

		unsigned int length;
		std::memcpy(&length, &payload[4], sizeof(length));

		for (unsigned int i = 0; i < length; i++) {
			unsigned int index, frequency;
			const unsigned int offset = 8 + (i * 8);

			std::memcpy(&index, &payload[offset], sizeof(index));
			std::memcpy(&frequency, &payload[offset + 4], sizeof(frequency));

			// AM presets
			if (index >= 0x0E && index <= 0x1C) {
				if (frequency == 0) {
					lastPresetAM = index;
				}
				else {
					presetAM.emplace(index, frequency);
				}
			}
			// FM presets
			else if (index >= 0x1D && index <= 0x2C) {
				if (frequency == 0) {
					lastPresetFM = index;
				}
				else {
					presetFM.emplace(index, frequency);
				}
			}
			// Other tuner settings
			else {
				switch (index) {
				case 0x00:
					mediaPlayer.tunerSteps = frequency;
					break;

				case 0x07:
					mediaPlayer.tunerRegion = frequency;
					break;
				}
				// Others TBD
			}

			// BUG BUG DEBUG
			if (debugSpew) {
				debugMessage = wxString::Format(
					"Id: %d. Setting: %02X, Value: %d",	messageId, index, frequency);
				debugSocket->SendTo(addrPeer, debugMessage, debugMessage.Length());
			}
		}

		break;
	}

	case 23: { 
		// Mute/Unmute
		// A3 99 17 80 01 Mute
		// A3 99 17 80 02 Unmute

		if (payload[4] == 0x01) {
			mediaPlayerDialog->SetMuteState(true);
		}
		else if (payload[4] == 0x02) {
			mediaPlayerDialog->SetMuteState(false);
		}

		// BUG BUG DEBUG
		if (debugSpew) {
			debugMessage = wxString::Format("Id: %d. Mute: %d",
				messageId, payload[4]);
			debugSocket->SendTo(addrPeer, debugMessage, debugMessage.Length());
		}

		break;
	}

	case 24: { 
		// Balance
		
		// A3 99 18 80 00 03
		// A3 99 18 80 00 FD
		// Signed, +ve Right Channel, -ve Left Channel
		// Ignore Zones for time being as no UI for individual zones, just Zone 0
		if (payload[4] == 0) {
			balance = payload[5];
			mediaPlayerDialog->SetBalance((balance & 0x80) == 0x80 ? balance - 256 : balance);
		}
		// BUG BUG DEBUG
		if (debugSpew) {
			debugMessage = wxString::Format("Id: %d. Balance: Zone: %d Value: %d", 
				messageId, payload[4], balance);
			debugSocket->SendTo(addrPeer, debugMessage, debugMessage.Length());
		}
		break;
	}

	case 26: { 
		// Sub Woofer
		// A3 99 1A 80 0C 0C 0C 00
		// Ignore for now, no UI
		//zone0SubWoofer = payload[4];
		//zone1SubWoofer = payload[5];
		//zone2SubWoofer = payload[6];

		// BUG BUG DEBUG
		if (debugSpew) {
			debugMessage = wxString::Format("Id: %d. Subwoofer: Zone 0 %d, Zone 1: %d, Zone 3: %d",
				messageId, payload[4], payload[5], payload[6]);
			debugSocket->SendTo(addrPeer, debugMessage, debugMessage.Length());
		}

		break;
	}

	case 27: { 
		// Tone Controls
		// A3 99 1B 80 03 08 05 03
		// Note values range from -15 to 15, so ensure all clients used a signed int
		// For Two's Complement 
		// bass = (payload[5] & 0x80) == 0x80 ? payload[5] - 256 : payload[5];
		// And reverse payload[5] = (value < 0) ? value + 256 : value;
		// Alternatively 
		// to an int static_cast<int>(static_cast<int8_t>(byte value)
		// to a byte static_cast<unsigned char>(static_cast<int8_t>(integer value));

		bass = payload[5];
		midrange = payload[6];
		treble = payload[7];
		mediaPlayerDialog->SetBass((bass & 0x80) == 0x80 ? bass - 256 : bass);
		mediaPlayerDialog->SetMidrange((midrange & 0x80) == 0x80 ? midrange -256 : midrange);
		mediaPlayerDialog->SetTreble((treble & 0x80) == 0x80 ? treble - 256 : treble);
		

		// BUG BUG DEBUG
		if (debugSpew) {
			debugMessage = wxString::Format("Id: %d. Tone, Zone: %d, Bass: %d, Midrange: %d, Treble: %d",
				messageId, payload[4], bass, midrange, treble);
			debugSocket->SendTo(addrPeer, debugMessage, debugMessage.Length());
		}

		break;
	}

	case 28: {
		// Volume Limit
		// A3 99 1C 80 0D 13 18 00
		mediaPlayer.zoneOneVolumeLimit = payload[4];
		mediaPlayer.zoneOneVolumeLimit = payload[5];
		mediaPlayer.zoneOneVolumeLimit = payload[6];
		// No UI for zone 4 which is carried in payload[7];
		break;
	}

	case 29: { 
		// Volume
		// A3 99 1D 80 01 0A 00 00
		// A3 99 1D 80 01 09 00 00

		zone0Volume = payload[4];
		zone1Volume = payload[5];
		
		mediaPlayerDialog->SetZone0Volume(zone0Volume);
		mediaPlayerDialog->SetZone1Volume(zone1Volume);

		// BUG BUG DEBUG
		if (debugSpew) {
			debugMessage = wxString::Format("Id: %d. Volume: Zone 0: %d, Zone 1: %d",
				messageId, payload[4], payload[5]);
			debugSocket->SendTo(addrPeer, debugMessage, debugMessage.Length());
		}

		break;
	}

	case 30: { 
		// Capabilities for each zone
		// A3 99 1E 80 07 00 07 00 1F 02 00 00 C0 01
		
		// BUG BUG Not used anywhere and not understood
		unsigned short zone0 = payload[4] | (payload[5] << 8);
		unsigned short zone1 = payload[6] | (payload[7] << 8);
		unsigned short zone2 = payload[8] | (payload[9] << 8);
		unsigned short zone3 = payload[10] | (payload[11] << 8);
		unsigned short global = payload[12] | (payload[13] << 8);

		break;
	}

	case 32: { 
		// Power State
		// A3 99 20 80 01 On
		// A3 99 20 80 02 Off

		// De bounce, if we are already on, no need to continually update the UI.
		if (payload[4] == 0x01) {
			if (!isOn) {
				isOn = true;
				mediaPlayerDialog->SetPowerState(isOn);
			}
		}

		// BUG BUG What if another device turns off the media player. What to display
		if (payload[4] == 0x02) {
			if (isOn) {
				isOn = false;
				mediaPlayerDialog->SetPowerState(isOn);
			}
		}

		// BUG BUG DEBUG
		if (debugSpew) {
			debugMessage = wxString::Format("Id: %d. Power: %d",
				messageId, payload[4]);
			debugSocket->SendTo(addrPeer, debugMessage, debugMessage.Length());
		}

		break;
	}

	case 33: {
		// Device Name
		// A3 99 21 80 08 43 4F 4E 20 42 52 49 4F 00

		mediaPlayer.deviceName.Clear();
		for (size_t i = 0; i < payload[4]; i++) {
			mediaPlayer.deviceName.append(1, payload[5 + i]);
		}

		auiManager->GetPane(PLUGIN_API_NAME).Caption(mediaPlayer.deviceName);
		auiManager->Update();

		// BUG BUG DEBUG
		if (debugSpew) {
			debugMessage = wxString::Format("Id: %d. Name: %s",
				messageId, mediaPlayer.deviceName);
			debugSocket->SendTo(addrPeer, debugMessage, debugMessage.Length());
		}

		break;
	}
	
	case 45: { 
		// Zone Names
		// A3 99 2D 80 00 06 53 41 4C 4F 4F 4E 00
		// A3 99 2D 80 01 07 43 4F 43 4B 50 49 54 00
		// A3 99 2D 80 02 06 5A 6F 6E 65 20 33 00;

		unsigned char zoneId;
		zoneId = payload[4];

		wxString zoneName;
		unsigned int zoneNameLength = payload[5];
		for (size_t i = 0; i < zoneNameLength; i++) {
			zoneName.append(1, payload[6 + i]);
		}

		if (zoneId == 0) {
			mediaPlayer.zoneOneName = zoneName;
			mediaPlayerDialog->SetZone0Name(mediaPlayer.zoneOneName);
		}

		if (zoneId == 1) {
			mediaPlayer.zoneTwoName = zoneName;
			mediaPlayerDialog->SetZone1Name(mediaPlayer.zoneTwoName);
		}

		if (zoneId == 2) {
			mediaPlayer.zoneThreeName = zoneName;
			// No UI for Zone3 volume, balance, tone etc.
		}

			
		break;
	}

	case 54: { 
		// Not yet determined. Look for something with following values
		// A3 99 36 80 06 00 88 13 01 (5000)
		// A3 99 36 80 07 00 88 13 01
		// A3 99 36 80 08 00 DC 05 01 (1500)

		// BUG BUG DEBUG
		if (debugSpew) {
			debugMessage = wxString::Format("Id: %d. Entry: %d, Value: %d,Index: %d\n", 
				messageId, payload[4] | (payload[5] << 8), payload[6] | (payload[7] << 8), 
				payload[8]);
			debugSocket->SendTo(addrPeer, debugMessage, debugMessage.Length());
		}

		break;
	}

	default: {
		// BUG BUG DEBUG
		if (debugSpew) {
			debugMessage = wxString::Format("Unsupported Id: %d.\n", messageId);
			for (size_t i = 4; i < payload.size(); i++) {
				debugMessage+= wxString::Format("%02X ", payload[i]);
			}
			debugSocket->SendTo(addrPeer, debugMessage, debugMessage.Length());
		}

		break;
	}

	} // end switch
}

// Part of the "handshake" when navigating folders & tracks 
// Unused
void MediaPlayerPlugin::SendAck(void) {
	wxCommandEvent* commandEvent = new wxCommandEvent(wxEVT_MEDIAPLAYER_DIALOG_EVENT, MEDIAPLAYER_ACK);
	wxQueueEvent(this, commandEvent);
}

// Part of the "handshake" when navigating folders & tracks 
// Unused
void MediaPlayerPlugin::SendConfirm(const int recordsReceived) {
	wxCommandEvent* commandEvent = new wxCommandEvent(wxEVT_MEDIAPLAYER_DIALOG_EVENT, MEDIAPLAYER_CONFIRM);
	commandEvent->SetInt(receivedRecords);
	wxQueueEvent(this, commandEvent);
}

// Event Handler for events received from the Media Player dialog
void MediaPlayerPlugin::OnDialogEvent(wxCommandEvent& event) {

	std::vector<unsigned char> payload;

	// BUG BUG DEBUG
	if (debugSpew) {
		debugMessage = wxString::Format("Event: %d, Int %d, String: %s",
			event.GetId(), event.GetInt(), event.GetString());
		debugSocket->SendTo(addrPeer, debugMessage.data(), debugMessage.Length());
	}

	switch (event.GetId()) {

	case MEDIAPLAYER_ON:
		if (event.GetString().Lower() == "on") {
			payload.push_back(0xA3);
			payload.push_back(0x99);
			payload.push_back(0x01); // #define POWER_ON 0x01
			payload.push_back(0x00); 
		}

		else if (event.GetString().Lower() == "off") {
			payload.push_back(0xA3);
			payload.push_back(0x99);
			payload.push_back(0x1C); // #define POWER_OFF 0x1C
			payload.push_back(0x00);
			payload.push_back(0x02); // #define OFF 0x02

			// Clear the UI
			mediaPlayerDialog->ClearFileList();
			mediaPlayerDialog->ClearSource();
			mediaPlayerDialog->SetStatus("Not connected");
		}
		break;

	case MEDIAPLAYER_MUTE:
		if (event.GetString().Lower() == "mute") {
			payload.push_back(0xA3);
			payload.push_back(0x99);
			payload.push_back(0x11); // #define CHANGE_MUTE 0x11
			payload.push_back(0x00);
			payload.push_back(0x01); // define MUTE 0x01
		}

		else {
			payload.push_back(0xA3);
			payload.push_back(0x99);
			payload.push_back(0x11); // #define CHANGE_MUTE 0x11
			payload.push_back(0x00);
			payload.push_back(0x02); // #define UNMUTE 0x02
		}
		break;

	case MEDIAPLAYER_ZONE0:
		payload.push_back(0xA3);
		payload.push_back(0x99);
		payload.push_back(0x18); // #define CHANGE_VOLUME 0x18
		payload.push_back(0x00);
		payload.push_back(0x00); // Zone Number
		payload.push_back(event.GetInt()); // Volume
		break;

	case MEDIAPLAYER_ZONE1:
		payload.push_back(0xA3);
		payload.push_back(0x99);
		payload.push_back(0x18); // #define CHANGE_VOLUME 0x18
		payload.push_back(0x00);
		payload.push_back(0x01); // Zone Number
		payload.push_back(event.GetInt()); // Volume
		break;

	case MEDIAPLAYER_BALANCE: {
		balance = (event.GetInt() < 0) ? event.GetInt() + 256 : event.GetInt();
		payload.push_back(0xA3);
		payload.push_back(0x99);
		payload.push_back(0x12); // #define CHANGE_BALANCE 0x12
		payload.push_back(0x00);
		payload.push_back(0x00); // Zone 0
		payload.push_back(balance); // Remember Two's complement

		break;
	}
	case MEDIAPLAYER_BASS:
		bass = static_cast<unsigned char>(static_cast<int8_t>(event.GetInt()));
		payload.push_back(0xA3);
		payload.push_back(0x99);
		payload.push_back(0x16); // #define CHANGE_TONE 0x16
		payload.push_back(0x00);
		payload.push_back(0x03); // System under test, all tone settings are Zone 3 (or Global) ?
		payload.push_back(bass); // Remember tone controls, like balance are Two's complement
		payload.push_back(midrange);
		payload.push_back(treble);
		break;

	case MEDIAPLAYER_MIDRANGE:
		midrange = static_cast<unsigned char>(static_cast<int8_t>(event.GetInt()));
		payload.push_back(0xA3);
		payload.push_back(0x99);
		payload.push_back(0x16); // #define CHANGE_TONE 0x16
		payload.push_back(0x00);
		payload.push_back(0x03); 
		payload.push_back(bass);
		payload.push_back(midrange);
		payload.push_back(treble);
		break;

	case MEDIAPLAYER_TREBLE:
		treble = static_cast<unsigned char>(static_cast<int8_t>(event.GetInt()));
		payload.push_back(0xA3);
		payload.push_back(0x99);
		payload.push_back(0x16); // #define CHANGE_TONE 0x16
		payload.push_back(0x00);
		payload.push_back(0x03); 
		payload.push_back(bass);
		payload.push_back(midrange);
		payload.push_back(treble);
		break;

	case MEDIAPLAYER_SHUFFLE: {
		// A3 99 0F 00 0A 00 00 00 01 00 00 00 - enable
		// A3 99 0F 00 0A 00 00 00 00 00 00 00 - disable
		int shuffle = event.GetInt();

		payload.push_back(0xA3);
		payload.push_back(0x99);
		payload.push_back(0x0F);
		payload.push_back(0x00);
		payload.push_back(0x0A);
		payload.push_back(0x00);
		payload.push_back(0x00);
		payload.push_back(0x00); // BUG BUG Set this from UI
		payload.push_back(0x00);
		payload.push_back(0x00);
		payload.push_back(0x00);

		break;
	}

	case MEDIAPLAYER_REPEAT: {
		// A3 99 0F 00 09 00 00 00 01 00 00 00 - enable
		// A3 99 0F 00 09 00 00 00 00 00 00 00 - disable
		int repeat = event.GetInt();

		payload.push_back(0xA3);
		payload.push_back(0x99);
		payload.push_back(0x0F);
		payload.push_back(0x00);
		payload.push_back(0x09);
		payload.push_back(0x00);
		payload.push_back(0x00);
		payload.push_back(0x00);
		payload.push_back(0x00); // BUG BUG Set this from UI
		payload.push_back(0x00);
		payload.push_back(0x00);
		payload.push_back(0x00);

		break;
	}
	case MEDIAPLAYER_LIST_DEVICE:
		// Get the Device top level folder;
		folderId = event.GetInt();

		payload.push_back(0xA3);
		payload.push_back(0x99); 
		payload.push_back(0x09); // #define REQUEST_FOLDER 0x09
		payload.push_back(0x00); 
		payload.push_back(sessionId);
		payload.push_back(folderId & 0xFF);
		payload.push_back((folderId >> 8) & 0xFF);
		payload.push_back((folderId >> 16) & 0xFF);
		payload.push_back((folderId >> 24) & 0xFF);
		payload.push_back(FUSION_FOLDER_REQUEST::Device); 
		payload.push_back(headUnitId);

		// BUG BUG DEBUG
		if (debugSpew) {
			debugMessage = wxString::Format("List Device; Session Id: %d, Head Unit Id: %d, Folder Id: %d",
				sessionId, headUnitId, folderId);
			debugSocket->SendTo(addrPeer, debugMessage, debugMessage.Length());
		}

		break;

	case MEDIAPLAYER_LIST_ROOT:
		// Get the root folder; Don't think this is used anywhere
		folderId = event.GetInt();

		payload.push_back(0xA3);
		payload.push_back(0x99);
		payload.push_back(0x09); // #define REQUEST_FOLDER 0x09
		payload.push_back(0x00); // Always 0x00 for a Head Unit
		payload.push_back(sessionId);
		payload.push_back(folderId & 0xFF);
		payload.push_back((folderId >> 8) & 0xFF);
		payload.push_back((folderId >> 16) & 0xFF);
		payload.push_back((folderId >> 24) & 0xFF);
		payload.push_back(FUSION_FOLDER_REQUEST::Root);
		payload.push_back(headUnitId);

		// BUG BUG DEBUG
		if (debugSpew) {
			debugMessage = wxString::Format("List Root; Session Id: %d, Head Unit Id: %d, Folder Id: %d",
				sessionId, headUnitId, folderId);
			debugSocket->SendTo(addrPeer, debugMessage, debugMessage.Length());
		}

		break;

	case MEDIAPLAYER_LIST_CURRENT:
		// Get the items of this folder; Don't think this is used anywhere
		folderId = event.GetInt();

		payload.push_back(0xA3);
		payload.push_back(0x99);
		payload.push_back(0x09); // #define REQUEST_FOLDER 0x09
		payload.push_back(0x00);
		payload.push_back(sessionId);
		payload.push_back(folderId & 0xFF);
		payload.push_back((folderId >> 8) & 0xFF);
		payload.push_back((folderId >> 16) & 0xFF);
		payload.push_back((folderId >> 24) & 0xFF);
		payload.push_back(FUSION_FOLDER_REQUEST::Bookmark);
		payload.push_back(headUnitId);

		// BUG BUG DEBUG
		if (debugSpew) {
			debugMessage = wxString::Format("List Current; Session Id: %d, Head Unit Id: %d, Folder Id: %d", 
				sessionId, headUnitId, folderId);
			debugSocket->SendTo(addrPeer, debugMessage, debugMessage.Length());
		}

		break;

	case MEDIAPLAYER_LIST_CHILD: 
		// Get the child items of the selected folder
		folderId = event.GetInt();

		// Get Children of this folder
		payload.push_back(0xA3);
		payload.push_back(0x99);
		payload.push_back(0x09); // #define REQUEST_FOLDER 0x09
		payload.push_back(0x00);
		payload.push_back(sessionId);
		payload.push_back(folderId & 0xFF);
		payload.push_back((folderId >> 8) & 0xFF);
		payload.push_back((folderId >> 16) & 0xFF);
		payload.push_back((folderId >> 24) & 0xFF);
		payload.push_back(FUSION_FOLDER_REQUEST::Child);
		payload.push_back(headUnitId);

		// BUG BUG DEBUG
		if (debugSpew) {
			debugMessage = wxString::Format("List Child; Session Id: %d, Head Unit Id: %d, Folder Id %d",
				sessionId, headUnitId, folderId);
			debugSocket->SendTo(addrPeer, debugMessage, debugMessage.Length());
		}

		break;

	case MEDIAPLAYER_LIST_PARENT:
		// Get Parent of the current folder

		payload.push_back(0xA3);
		payload.push_back(0x99);
		payload.push_back(0x09); // #define REQUEST_FOLDER 0x09
		payload.push_back(0x00);
		payload.push_back(sessionId);
		payload.push_back(0x00);
		payload.push_back(0x00);
		payload.push_back(0x00);
		payload.push_back(0x00);
		payload.push_back(FUSION_FOLDER_REQUEST::Parent);
		payload.push_back(headUnitId);

		// BUG BUG DEBUG
		if (debugSpew) {
			debugMessage = wxString::Format("List Parent; Session Id: %d, Head Unit Id: %d, Folder Id %d",
				sessionId, headUnitId, folderId);
			debugSocket->SendTo(addrPeer, debugMessage, debugMessage.Length());
		}

		break;

	case MEDIAPLAYER_NEXT_TRACK:
		if ((sessionId == FUSION_MEDIA_PORT::am) || (sessionId == FUSION_MEDIA_PORT::fm)) {
			// Scan to the next frequency
			// A3 99 05 00 01 02 80 04 39 06
			// BUG BUG DAB ?
			payload.push_back(0xA3);
			payload.push_back(0x99);
			payload.push_back(0x05); // #define CHANGE_FREQUENCY 0x05
			payload.push_back(0x00);
			payload.push_back(sessionId);
			payload.push_back(0x01); // #define NEXT_CHANNEL 0x01
			payload.push_back(frequency & 0xFF);
			payload.push_back((frequency >> 8) & 0xFF);
			payload.push_back((frequency >> 16) & 0xFF);
			payload.push_back((frequency >> 24) & 0xFF);
		}

		else if ((sessionId == FUSION_MEDIA_PORT::usb) || (sessionId == FUSION_MEDIA_PORT::mtp) || (sessionId == FUSION_MEDIA_PORT::ipod)) {
			// Play the next track
			// A3 99 03 00 05 04
			payload.push_back(0xA3);
			payload.push_back(0x99);
			payload.push_back(0x03); // #define SELECT_TRACK 0x03
			payload.push_back(0x00);
			payload.push_back(sessionId);
			payload.push_back(0x04); // #define NEXT_TRACK 0x04

		}
		break;

	case MEDIAPLAYER_PAUSE:

		if ((sessionId == FUSION_MEDIA_PORT::usb) || (sessionId == FUSION_MEDIA_PORT::mtp) || (sessionId == FUSION_MEDIA_PORT::ipod)) {

			if (event.GetString().Lower() == "play") {
				payload.push_back(0xA3);
				payload.push_back(0x99);
				payload.push_back(0x03); // define PAUSE_PLAY 0x03
				payload.push_back(0x00);
				payload.push_back(sessionId);
				payload.push_back(0x01); // #define PLAY 0x01
			}
			else if (event.GetString().Lower() == "pause") {
				payload.push_back(0xA3);
				payload.push_back(0x99);
				payload.push_back(0x03); // #define PAUSE_PLAY 0x03
				payload.push_back(0x00);
				payload.push_back(sessionId);
				payload.push_back(0x02); // #define PAUSE 0x02
			}
		}

		// For am & fm radio, the "Play/Pause" button serves double duty and saves 
		// the currently tuned station to the list of presets
		// BUG BUG. Assuming upper limit of 15 presets given the range of indexes
		else if ((sessionId == FUSION_MEDIA_PORT::am) || (sessionId == FUSION_MEDIA_PORT::fm)) {
			
			if (event.GetString().Lower() == "save") {
				// Increment the next preset index value and save the frequency
				unsigned int index;
				if (sessionId == FUSION_MEDIA_PORT::fm) {
					// BUG BUG Doesn't accomodate "holes"
					index = lastPresetFM;
					if (lastPresetFM < 0x2D) {
						lastPresetFM++;
					}
					presetFM.emplace(index, frequency);
					// Update the UI
					mediaPlayerDialog->ClearFileList();
					for (auto it : presetFM) {
						mediaPlayerDialog->AppendRadioStation(wxString::Format("%.1f MHz", it.second * 1e-6), it.second, it.first);
					}
				}
				else if (sessionId == FUSION_MEDIA_PORT::am) {
					index = lastPresetAM;
					if (lastPresetAM < 0x1D) {
						lastPresetAM++;
					}
					presetAM.emplace(index, frequency);
					mediaPlayerDialog->ClearFileList();
					for (auto it : presetAM) {
						mediaPlayerDialog->AppendRadioStation(wxString::Format("%.1f KHz", it.second * 1e-3), it.second, it.first);
					}
				}
				else {
					// Presumably can save DAB stations ??
					break;
				}
								
				// Save a Preset radio station
				payload.push_back(0xA3);
				payload.push_back(0x99);
				payload.push_back(0x0F); // #define SAVE_PRESET
				payload.push_back(0x00);
				payload.push_back(index);
				payload.push_back((index >> 8) & 0xFF);
				payload.push_back((index >> 16) & 0xFF);
				payload.push_back((index >> 24) & 0xFF);
				payload.push_back(frequency & 0xFF);
				payload.push_back((frequency >> 8) & 0xFF);
				payload.push_back((frequency >> 16) & 0xFF);
				payload.push_back((frequency >> 24) & 0xFF);
			}
		}

		break;

	case MEDIAPLAYER_PREVIOUS_TRACK:
		if ((sessionId == FUSION_MEDIA_PORT::am) || (sessionId == FUSION_MEDIA_PORT::fm)) {
			
			// A3 99 05 00 01 02 80 04 39 06
			// Scan backwards to the next frequency

			payload.push_back(0xA3);
			payload.push_back(0x99);
			payload.push_back(0x05); // #define CHANGE_FREQUENCY 0x05
			payload.push_back(0x00);
			payload.push_back(sessionId);
			payload.push_back(0x03); // #define PREVIOUS_CHANNEL 0x03
			payload.push_back(frequency & 0xFF);
			payload.push_back((frequency >> 8) & 0xFF);
			payload.push_back((frequency >> 16) & 0xFF);
			payload.push_back((frequency >> 24) & 0xFF);
		}

		else if ((sessionId == FUSION_MEDIA_PORT::usb) || (sessionId == FUSION_MEDIA_PORT::mtp) || (sessionId == FUSION_MEDIA_PORT::ipod)) {

			// Play the previous track
			// A3 99 03 00 05 06
			payload.push_back(0xA3);
			payload.push_back(0x99);
			payload.push_back(0x03); // #define SELECT_TRACK 0x03
			payload.push_back(0x00);
			payload.push_back(sessionId);
			payload.push_back(0x06); // #define PREVIOUS_TRACK 0x06
		}
		break;

	case MEDIAPLAYER_SELECT_PRESET: {
		// Goto the preset frequency
		// A3 99 05 00 01 05 E0 59 43 05
		int presetFrequency = event.GetInt();
		payload.push_back(0xA3);
		payload.push_back(0x99);
		payload.push_back(0x05); // #define CHANGE_FREQUENCY 0x05
		payload.push_back(0x00);
		payload.push_back(sessionId);
		payload.push_back(0x05); // #define PRESET_CHANNEL 0x05
		payload.push_back(presetFrequency & 0xFF);
		payload.push_back((presetFrequency >> 8) & 0xFF);
		payload.push_back((presetFrequency >> 16) & 0xFF);
		payload.push_back((presetFrequency >> 24) & 0xFF);

		break;
	}

	case MEDIAPLAYER_DELETE_PRESET: {
		// A3 99 0F 00 0E 00 00 00 00 00 00 00
		unsigned int presetValue = event.GetInt();
		payload.push_back(0xA3);
		payload.push_back(0x99);
		payload.push_back(0x0F); // #define DELETE_PRESET 0x0F
		payload.push_back(0x00);
		payload.push_back(presetValue & 0xFF);
		payload.push_back((presetValue >> 8) & 0xFF);
		payload.push_back((presetValue >> 16) & 0xFF);
		payload.push_back((presetValue >> 24) & 0xFF);
		payload.push_back(0x00);
		payload.push_back(0x00);
		payload.push_back(0x00);
		payload.push_back(0x00);

		// Update the UI
		// BUG BUG Deduplicate
		if (sessionId == FUSION_MEDIA_PORT::am) {
			mediaPlayerDialog->ClearFileList();
			presetAM.erase(presetValue);
			if (!presetAM.empty()) {
				for (auto it : presetAM) {
					mediaPlayerDialog->AppendRadioStation(wxString::Format("%.1f KHz", it.second * 1e-3), it.second, it.first);
				}
			}
		}
		if (sessionId == FUSION_MEDIA_PORT::fm) {
			mediaPlayerDialog->ClearFileList();
			presetFM.erase(presetValue);
			if (!presetFM.empty()) {
				for (auto it : presetFM) {
					mediaPlayerDialog->AppendRadioStation(wxString::Format("%.1f MHz", it.second * 1e-6), it.second, it.first);
				}
			}
		}
		break;
	}

	case MEDIAPLAYER_DELETE_ALLPRESET: {
		// Simply iterate through all of the preset values
		if (event.GetInt() == 0) {
			for (auto it: presetAM) {
				wxCommandEvent *commandEvent = new wxCommandEvent(wxEVT_MEDIAPLAYER_DIALOG_EVENT, MEDIAPLAYER_DELETE_PRESET);
				commandEvent->SetInt(it.first);
				wxQueueEvent(this,commandEvent);
			}
		}
		else {
			for (auto it : presetFM) {
				wxCommandEvent* commandEvent = new wxCommandEvent(wxEVT_MEDIAPLAYER_DIALOG_EVENT, MEDIAPLAYER_DELETE_PRESET);
				commandEvent->SetInt(it.first);
				wxQueueEvent(this, commandEvent);
			}
		}
		break;
	}

	case MEDIAPLAYER_NEXT_FOLDER:
		// Called by RequestChildFolder - currently unused
		// A3 99 09 00 05 00 00 00 00 01 02

		// 0 just gets message 16, ?
		// 1 is get current folder,
		// 2 is next
		// 3 is back

		payload.push_back(0xA3);
		payload.push_back(0x99);
		payload.push_back(0x09); // #define REQUEST_FOLDER 0x09
		payload.push_back(0x00);
		payload.push_back(sessionId);
		payload.push_back(folderId & 0xFF);
		payload.push_back((folderId >> 8) & 0xFF);
		payload.push_back((folderId >> 16) & 0xFF);
		payload.push_back((folderId >> 24) & 0xFF);
		payload.push_back(FUSION_FOLDER_REQUEST::Child);
		payload.push_back(headUnitId);

		break;

	case MEDIAPLAYER_PREVIOUS_FOLDER:
		// Called by RequestParentFolder and RequestRootFolder, both unused
		// A3 99 09 00 05 00 00 00 00 01 02

		// 0 just gets message 16, ?
		// 1 is get current folder,
		// 2 is next
		// 3 is back

		payload.push_back(0xA3);
		payload.push_back(0x99);
		payload.push_back(0x09); // #define REQUEST_FOLDER 0x09
		payload.push_back(0x00);
		payload.push_back(sessionId);
		payload.push_back(folderId & 0xFF);
		payload.push_back((folderId >> 8) & 0xFF);
		payload.push_back((folderId >> 16) & 0xFF);
		payload.push_back((folderId >> 24) & 0xFF);
		payload.push_back(FUSION_FOLDER_REQUEST::Parent);
		payload.push_back(headUnitId);

		break;

	case MEDIAPLAYER_SOURCE: {
		// Switch to the selected media source
		// BUG BUG For Bluetooth, save the previous session ID.
		// Start a 2' timer and if no connection made, revert to original source
		// Detect an active BT connection with
		// Msg Id: 4
		// Session Id: 8 (BT)
		// Status : 2 (paused)
		// Track Number 255
		// Total Tracks : 255
		// Track Length : 0

		wxString sourceName;
		sourceName = event.GetString();
		payload.push_back(0xA3);
		payload.push_back(0x99);
		payload.push_back(0x02);  // #define CHANGE_SOURCE 0x02
		payload.push_back(0x00);
		payload.push_back(LookupSourceId(sourceName));

		break;
	}

	case MEDIAPLAYER_ACK:
		// Send the acknowledgement when navigating folders and tracks
		//A3 99 0A 00 05 02
		payload.push_back(0xA3);
		payload.push_back(0x99);
		payload.push_back(0x0A); // #define SEND_ACK 0x0A
		payload.push_back(0x00); 
		payload.push_back(sessionId);
		payload.push_back(headUnitId);

		break;

	case MEDIAPLAYER_CONFIRM:
		receivedRecords = event.GetInt();
		// Send Confirmation
		// A3 99 0B 00 05 00 00 00 00 01 00 00 00 02
		// BUG BUG Could use paging for very large numbers
		payload.push_back(0xA3);
		payload.push_back(0x99);
		payload.push_back(0x0B); // #define SEND_CONFIRM 0x0B
		payload.push_back(0x00);
		payload.push_back(sessionId);
		payload.push_back(0x00);
		payload.push_back(0x00);
		payload.push_back(0x00);
		payload.push_back(0x00);
		payload.push_back(receivedRecords & 0xFF);
		payload.push_back((receivedRecords >> 8) & 0xFF);
		payload.push_back((receivedRecords >> 16) & 0xFF);
		payload.push_back((receivedRecords >> 24) & 0xFF);
		payload.push_back(headUnitId);

		break;

	case MEDIAPLAYER_FILE_PLAY:
		// Play a music track when selected....
		folderId = event.GetInt();
		payload.push_back(0xA3);
		payload.push_back(0x99);
		payload.push_back(0x09); // #define SEND_PLAY 0x09
		payload.push_back(0x00);
		payload.push_back(sessionId);
		payload.push_back(folderId & 0xFF);
		payload.push_back((folderId >> 8) & 0xFF);
		payload.push_back((folderId >> 16) & 0xFF);
		payload.push_back((folderId >> 24) & 0xFF);
		payload.push_back(FUSION_FOLDER_REQUEST::Child);
		payload.push_back(headUnitId);

		break;

	case MEDIAPLAYER_SET_DEVICE_NAME: {
		// Set the device name
		payload.push_back(0xA3);
		payload.push_back(0x99);
		payload.push_back(0x1D); // #define SET_DEVICE_NAME 0x1D
		payload.push_back(0x00);
		payload.push_back(event.GetString().size());
		for (auto c : event.GetString()) {
			payload.push_back(c);
		}	
		payload.push_back(0x00);

		break;
	}

	case MEDIAPLAYER_SET_ZONE_NAME: {
		// Set a zone name
		payload.push_back(0xA3);
		payload.push_back(0x99);
		payload.push_back(0x22); // #define SET_ZONE_NAME 0x22
		payload.push_back(0x00);
		payload.push_back(event.GetInt() & 0xFF); // Zone Number
		payload.push_back(event.GetString().size());
		for (auto c : event.GetString()) {
			payload.push_back(c);
		}
		payload.push_back(0x00);

		break;

	}

	case MEDIAPLAYER_SET_VOLUME_LIMIT: {
		// Set the maximum volume for a zone
		payload.push_back(0xA3);
		payload.push_back(0x99);
		payload.push_back(0x17); // #define SET_VOLUME_LIMIT 0x17
		payload.push_back(0x00);
		payload.push_back(event.GetInt() & 0xFF); // Zone Number
		payload.push_back(atoi(event.GetString()) & 0xFF); // Volume Limit
		
		break;
	}

	case MEDIAPLAYER_SET_TUNER_REGION: {
		// Change the tuner region (Note looks like it also deletes the presets)
		payload.push_back(0xA3);
		payload.push_back(0x99);
		payload.push_back(0x0F); // Hmmm Same as Delete Preset ??
		payload.push_back(0x00);
		payload.push_back(0x07); // Value = 7 for tuner region
		payload.push_back(0x00);
		payload.push_back(0x00);
		payload.push_back(0x00);
		payload.push_back(event.GetInt() & 0xFF);
		payload.push_back(0x00);
		payload.push_back(0x00);
		payload.push_back(0x00);

		break;
	}

	case MEDIAPLAYER_SET_TUNER_STEP:
		// Change the tuner step frequencies
		payload.push_back(0xA3);
		payload.push_back(0x99);
		payload.push_back(0x0F); // Hmmm Same as Delete Preset ??
		payload.push_back(0x00);
		payload.push_back(0x00); // Value = 0 for tuner steps
		payload.push_back(0x00);
		payload.push_back(0x00);
		payload.push_back(0x00);
		payload.push_back(event.GetInt() & 0xFF); //0 = Off, 1 = 100 2 = 200, 3 = 400, 4 = 800
		payload.push_back(0x00);
		payload.push_back(0x00);
		payload.push_back(0x00);

		break;

	// Unused entries
	/*
	case MEDIAPLAYER_DEBUG3: {
		// Send a probe
		// A3 99 0C 00 5E 06 00 00
		payload.push_back(0xA3);
		payload.push_back(0x99);
		payload.push_back(0x0C);
		payload.push_back(0x00);
		payload.push_back(0x5E);
		payload.push_back(0x06);
		payload.push_back(0x00);
		payload.push_back(0x00);

		break;
	}
	*/
	default:
		event.Skip();
		break;
	}

	if (payload.size() > 0) {

		// BUG BUG DEBUG
		if (debugSpew) {
			debugMessage = "\n";
			for (auto it: payload) {
				debugMessage+=wxString::Format("%02X ", it);
			}
			debugMessage += "\n";
			debugSocket->SendTo(addrPeer, debugMessage.data(), debugMessage.size());
		}

		// Transmit the generated PGN onto the network
		auto sharedPointer = std::make_shared<std::vector<unsigned char>>(payload);
		auto result = WriteCommDriverN2K(nmea2000Driver,126720,0xFF,6,sharedPointer);
		if (result != CommDriverResult::RESULT_COMM_NO_ERROR) {
			// BUG BUG Should log the message. Or if we exceed a specific number of failures
		}
	}
}

// Look up the name of a media source given its index
wxString MediaPlayerPlugin::LookupSourceName(unsigned char sourceId) {
	auto it = mediaDevices.find(sourceId);
	if (it != mediaDevices.end()) {
		return it->second.deviceName;
	}
	else {
		return wxEmptyString;
	}
}

// Lookup the index of a media source given its name
unsigned char MediaPlayerPlugin::LookupSourceId(const wxString& sourceName) {
	for (auto it : mediaDevices) {
		if (it.second.deviceName == sourceName) {
			return it.first;
		}
	}
	return wxNOT_FOUND;
}

// Whenever the source is changed, this configures the UI
void MediaPlayerPlugin::UpdateUI(const unsigned char session) {

	wxString source = LookupSourceName(session);
	mediaPlayerDialog->SetSource(source);

	// Empty the tree control when we are changing sources
	mediaPlayerDialog->ClearFileList();

	// Clear everything let the setters do their job
	mediaPlayerDialog->SetGaugeRange(100);
	mediaPlayerDialog->SetGaugeValue(0);
	mediaPlayerDialog->SetStatus(wxEmptyString);

	// If we've switched to a USB device, retrieve and display the media folders and music track names
	// BUG BUG iPod support. I don't have one to test against
	if ((source.Lower() == "usb") || (source.Lower() == "mtp") || (source.Lower() == "ipod")) {
		wxCommandEvent* commandEvent = new wxCommandEvent(wxEVT_MEDIAPLAYER_DIALOG_EVENT, MEDIAPLAYER_LIST_ROOT);
		commandEvent->SetInt(0);
		wxQueueEvent(this, commandEvent);
	}

	// If we've switched to fm radio, display the list of presets
	else if (source.Lower() == "fm") {
		if (!presetFM.empty()) {
			for (auto it : presetFM) {
				mediaPlayerDialog->AppendRadioStation(wxString::Format("%.1f MHz", it.second * 1e-6), it.second, it.first);
			}
		}
	}

	// Similarly, if we've switched to am radio, retrieve the list of preset stations
	else if (source.Lower() == "am") {
		if (!presetAM.empty()) {
			for (auto it : presetAM) {
				mediaPlayerDialog->AppendRadioStation(wxString::Format("%.1f KHz", it.second * 1e-3), it.second, it.first);
			}
		}
	}
	else {
		// No idea what to display for dab, dvd, sirius
	}
}

