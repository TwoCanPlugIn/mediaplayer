// Copyright(C) 2018-2025 by Steven Adler
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
// along with the Racing plugin for OpenCPN. If not, see <https://www.gnu.org/licenses/>.
//
// NMEA2000® is a registered trademark of the National Marine Electronics Association

#ifndef OPTIONS_PAGES_H
#define OPTIONS_PAGES_H

// Pre compiled headers 
#include "wx/wxprec.h"

#ifndef WX_PRECOMP
      #include <wx/wx.h>
#endif

//STL
#include <vector>
#include <algorithm>
#include <memory>
#include <queue>
#include <utility>

#include <wx/intl.h>
#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/spinctrl.h>
#include <wx/sizer.h>
#include <wx/checkbox.h>
#include <wx/statbox.h>
#include <wx/button.h>
#include <wx/dialog.h>
#include <wx/scrolwin.h>
#include <wx/event.h>

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
extern const int MEDIAPLAYER_NEXT_FOLDER;
extern const int MEDIAPLAYER_PREVIOUS_FOLDER;
extern const int MEDIAPLAYER_CURRENT_FOLDER;
extern const int MEDIAPLAYER_FILE_PLAY;
extern const int MEDIAPLAYER_SHUFFLE;
extern const int MEDIAPLAYER_REPEAT;
extern const int MEDIAPLAYER_LIST_ROOT;
extern const int MEDIAPLAYER_LIST_CURRENT;
extern const int MEDIAPLAYER_LIST_PARENT;
extern const int MEDIAPLAYER_LIST_CHILD;
extern const int MEDIAPLAYER_LIST_DEVICE;
extern const int MEDIAPLAYER_SAVE_PRESET;
extern const int MEDIAPLAYER_VOLUME;
extern const int MEDIAPLAYER_PROBE;
extern const int MEDIAPLAYER_ACK;
extern const int MEDIAPLAYER_CONFIRM;
extern const int MEDIAPLAYER_SELECT_PRESET;
extern const int MEDIAPLAYER_DELETE_PRESET;
extern const int MEDIAPLAYER_DELETE_ALLPRESET;
extern const int MEDIAPLAYER_SET_DEVICE_NAME;
extern const int MEDIAPLAYER_SET_ZONE_NAME;
extern const int MEDIAPLAYER_SET_VOLUME_LIMIT;
extern const int MEDIAPLAYER_SET_TUNER_REGION;
extern const int MEDIAPLAYER_SET_TUNER_STEP;
extern const int MEDIAPLAYER_JOB_APPEND_TEXT;
extern const int MEDIAPLAYER_JOB_INCREMENT_COUNTER;

struct FUSION_MEDIA_PLAYER {
	wxString deviceName;
	wxString playerVersion;
	wxString linkVersion;
	wxString zoneOneName;
	int zoneOneVolumeLimit;
	int zoneTwoVolumeLimit;
	int zoneThreeVolumeLimit;
	wxString zoneTwoName;
	wxString zoneThreeName;
	int tunerRegion;
	int tunerSteps;
	bool autoTune;
};

//  A whole series of classes that implement different settings pages
class  SettingsTemplate  : public wxPanel {

public:
	using EventSubmitter = std::function<void(std::unique_ptr<wxEvent>)>;
	
	// The constructor
	SettingsTemplate(wxScrolledWindow* parent, wxFlexGridSizer* sizer, wxString name, EventSubmitter submitter);

	// and destructor
	~SettingsTemplate(void);

	// Filters the tree list to only dislay the pages containing controls that match
	// the search term
	void ApplyFilter(const wxString& filter);

protected:
	// Associate a search term with one or two controls (e.g. text ctrl, spin control, slider)
	void Register(const wxString& searchText, wxWindow* first, wxWindow* second = nullptr);
	
	// Queue changes to settings as wxCommand Events
	// Same event mechanism used to control the media player
	void SubmitEvent(std::unique_ptr<wxEvent> event); // {
		//m_submitter(std::move(event));
	//}

private:
	wxScrolledWindow* parentWindow;
	wxFlexGridSizer* panelSizer;
	wxString panelName;

	// A search term can be associated with one or two controls on a settings page
	struct SearchItem {
		wxString text;
		wxWindow* first;
		wxWindow* second;
	};

	// The list of search terms for the entire settings panel (ie all of its controls)
	// Each panel "registers" its set of search terms
	std::vector<SearchItem> searchItems;

	// Once a filter is applied, scroll to he selected panel
	void ScrollTo(wxWindow* w);

	// Convoluted path to post wxCommand events to the plugin
	EventSubmitter eventSubmitter;
	
};

// Displays settings for the tuner region, tuning steps and auto/manual
class  TunerSettings : public SettingsTemplate {

public:
	// The constructor
	TunerSettings(wxScrolledWindow* parent, wxFlexGridSizer* sizer, wxString name, EventSubmitter submitter);

	// and destructor
	~TunerSettings(void);

	// Associates search terms with a pages controls
	// Eg.[0] = page title
	// [n] a search term associated with a control (wxTextCtrl, wxSliderCtrl, wxCheckBox)
	static const std::vector<wxString>& SearchTerms();

	using SettingsTemplate::SettingsTemplate;

protected:


private:

	wxStaticText* labelRegion;
	wxChoice* choiceRegion;
	wxStaticText* labelStep;
	wxChoice* choiceStep;
	wxCheckBox* checkDeleteFM;
	wxCheckBox* checkDeleteAM;

	void OnRegionChanged(wxCommandEvent& event);
	void OnStepChanged(wxCommandEvent& event);
	void OnFMChanged(wxCommandEvent& event);
	void OnAMChanged(wxCommandEvent& event);
};


// A panel displaying the device's top level settings
class  DeviceSettings : public SettingsTemplate {

public:
	// The constructor
	DeviceSettings(wxScrolledWindow* parent, wxFlexGridSizer* sizer, wxString name, EventSubmitter submitter);
	
	// and destructor
	~DeviceSettings(void);

	static const std::vector<wxString>& SearchTerms();

	using SettingsTemplate::SettingsTemplate;

protected:

	
private: 
	wxStaticText* labelDeviceName;
	wxTextCtrl* textDeviceName;
	wxStaticText* labelPlayerVersion;
	wxStaticText* labelLinkVersion;
	
	void OnDeviceNameChanged(wxCommandEvent& event);
	
};

class  ZoneSettings : public SettingsTemplate {

public:
	// The constructor
	ZoneSettings(wxScrolledWindow* parent, wxFlexGridSizer* sizer, wxString name, EventSubmitter submitter);

	// and destructor
	~ZoneSettings(void);

	static const std::vector<wxString>& SearchTerms();

	using SettingsTemplate::SettingsTemplate;

protected:


private:
	wxStaticText* labelZoneName;
	wxTextCtrl* textZoneName;
	wxStaticText* labelVolumeLimit;
	wxSlider* sliderVolumeLimit;
	
	int zoneNumber;

	void OnZoneNameChanged(wxCommandEvent& event);
	void OnVolumeLimitChanged(wxCommandEvent& event);
};

// A panel displaying a single zone's settings
class  AllZoneSettings : public SettingsTemplate {

public:
	// The constructor
	AllZoneSettings(wxScrolledWindow* parent, wxFlexGridSizer* sizer, wxString name, EventSubmitter submitter);

	// and destructor
	~AllZoneSettings(void);

	// Associate controls with the search terms
	static const std::vector<wxString>& SearchTerms();

	using SettingsTemplate::SettingsTemplate;

protected:

private:
	wxCheckBox* checkLink;
	void OnLinkChanged(wxCommandEvent& event);

};


#endif 
