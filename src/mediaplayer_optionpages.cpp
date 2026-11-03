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
// along with the Fusion Media Plyer plugin for OpenCPN. If not, see <https://www.gnu.org/licenses/>.
//

//
// Project: Fusion Media Player Plugin
// Description: New layout for settings, Similar to Visual Studio
// Owner: twocanplugin@hotmail.com
// Date: 10/10/2025
#include "mediaplayer_optionpages.h"

#include "search_util.h"

#pragma once

#include <wx/event.h>

// Unused....
class CommandEventQueue {
public:
	void Push(wxEvtHandler* target, const wxCommandEvent& event) {
		if (target == nullptr)
			return;

		events_.push(EventItem{
			target,
			std::unique_ptr<wxEvent>(event.Clone())
			});
	}

	bool ProcessOne() {
		if (events_.empty())
			return false;

		EventItem item = std::move(events_.front());
		events_.pop();

		item.target->ProcessEvent(*item.event);
		return true;
	}

	void ProcessAll() {
		while (ProcessOne()) {
		}
	}

	bool Empty() const {
		return events_.empty();
	}

	std::size_t Size() const {
		return events_.size();
	}

	void Clear() {
		while (!events_.empty())
			events_.pop();
	}

private:
	struct EventItem
	{
		wxEvtHandler* target;
		std::unique_ptr<wxEvent> event;
	};

	std::queue<EventItem> events_;
};

// Usage
// commandQueue.Push(this, event);

// Some global properties of the fusion media player
FUSION_MEDIA_PLAYER mediaPlayer;

// SettingsTemplate
// Base class from which all pages are derived
// Each page registers its set of key words

SettingsTemplate::SettingsTemplate(wxScrolledWindow* parent, wxFlexGridSizer* sizer, wxString name, EventSubmitter submitter):
	parentWindow(parent),
	panelSizer(sizer),
	panelName(name),
	eventSubmitter(std::move(submitter)) {

	sizer->SetVGap(10);
}

SettingsTemplate::~SettingsTemplate() {

}

void SettingsTemplate::Register(const wxString& searchText, wxWindow* first, wxWindow* second) {
	searchItems.push_back({ searchText, first, second });
}

void SettingsTemplate::ApplyFilter(const wxString& filter) {
	if (filter.IsEmpty()) {
		return;
	}

	for (const SearchItem& item : searchItems) {
		if (MatchesFilter(item.text, filter)) {
			parentWindow->Layout();
			parentWindow->FitInside();
			ScrollTo(item.first);
			return;
		}
	}
}


void SettingsTemplate::ScrollTo(wxWindow* w) {
	const wxPoint p = parentWindow->CalcUnscrolledPosition(w->GetPosition());

	int ppuX = 0, ppuY = 0;
	parentWindow->GetScrollPixelsPerUnit(&ppuX, &ppuY);

	if (ppuY > 0) {
		// Leave one scroll unit of context above the match
		parentWindow->Scroll(-1, std::max(0, p.y / ppuY - 1));
	}
}

// Any changed settings, post an especially crafted wxCommand Event
// Using the same event mechanism that the remote control dialog uses
void SettingsTemplate::SubmitEvent(std::unique_ptr<wxEvent> event) {
	eventSubmitter(std::move(event));
}

// Tuner Settings
// Allow the user to select the region, tuning steps and clear stored preset stations

const std::vector<wxString>& TunerSettings::SearchTerms() {
	// Search terms for the tuner
	static const std::vector<wxString> terms = {
		_("Tuner"),         // [0] page title
		_("Region"),        // [1]
		_("USA"),           // [2]
		_("Europe"),        // [3]
		_("Japan"),         // [4]
		_("Australasia"),   // [5]
		_("Step"),          // [6]
		_("Delete"),		// [7]
		_("Preset")			// [8]
	};
	return terms;
}

TunerSettings::TunerSettings(wxScrolledWindow* parent, wxFlexGridSizer* sizer, wxString name, EventSubmitter submitter) : 
	SettingsTemplate(parent, sizer, name, submitter) {

	labelRegion = new wxStaticText(parent, wxID_ANY, _("Select Tuner Region"), wxDefaultPosition, wxDefaultSize, 0);
	labelRegion->Wrap(-1);
	sizer->Add(labelRegion, 0, wxEXPAND, 0);

	wxString choiceRegionChoices[] = { _("USA"), _("Europe"), _("Japan"), _("Australasia") };
	int choiceRegionNChoices = sizeof(choiceRegionChoices) / sizeof(wxString);
	choiceRegion = new wxChoice(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, choiceRegionNChoices, choiceRegionChoices, 0);
	sizer->Add(choiceRegion, 0, wxEXPAND, 0);

	labelStep = new wxStaticText(parent, wxID_ANY, _("Select tuning steps"), wxDefaultPosition, wxDefaultSize, 0);
	labelStep->Wrap(-1);
	sizer->Add(labelStep, 0, wxEXPAND, 0);

	wxString choiceStepChoices[] = { _("Off"), _("100"), _("200"), _("400"), _("800") };
	int choiceStepNChoices = sizeof(choiceStepChoices) / sizeof(wxString);
	choiceStep = new wxChoice(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, choiceStepNChoices, choiceStepChoices, 0);
	sizer->Add(choiceStep, 0, wxEXPAND, 0);

	checkDeleteAM = new wxCheckBox(parent, wxID_ANY, "Delete all AM Preset stations ", wxDefaultPosition, wxDefaultSize);
	sizer->Add(checkDeleteAM, 0, wxEXPAND, 0);

	checkDeleteFM = new wxCheckBox(parent, wxID_ANY, "Delete all FM Preset stations ", wxDefaultPosition, wxDefaultSize);
	sizer->Add(checkDeleteFM, 0, wxEXPAND, 0);

	// Connect the event handlers
	choiceStep->Connect(wxEVT_CHOICE, wxCommandEventHandler(TunerSettings::OnStepChanged), NULL, this);
	choiceRegion->Connect(wxEVT_CHOICE, wxCommandEventHandler(TunerSettings::OnRegionChanged), NULL, this);
	checkDeleteAM->Connect(wxEVT_CHECKBOX, wxCommandEventHandler(TunerSettings::OnAMChanged), NULL, this);
	checkDeleteFM->Connect(wxEVT_CHECKBOX, wxCommandEventHandler(TunerSettings::OnFMChanged), NULL, this);

	// Populate initial values
	choiceRegion->SetSelection(mediaPlayer.tunerRegion);
	choiceStep->SetSelection(mediaPlayer.tunerSteps);

	// Associate controls with the search terms
	const std::vector<wxString>& t = SearchTerms();
	Register(t[1], labelRegion, choiceRegion);	// Region
	Register(t[6], labelStep, choiceStep);		// Step
	Register(t[7], checkDeleteAM, checkDeleteFM);	// Delete
	Register(t[8], checkDeleteAM, checkDeleteFM);	// Preset
}

TunerSettings::~TunerSettings() {
	choiceStep->Disconnect(wxEVT_CHOICE, wxCommandEventHandler(TunerSettings::OnStepChanged), NULL, this);
	choiceRegion->Disconnect(wxEVT_CHOICE, wxCommandEventHandler(TunerSettings::OnRegionChanged), NULL, this);
	checkDeleteAM->Disconnect(wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler(TunerSettings::OnAMChanged), NULL, this);
	checkDeleteFM->Disconnect(wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler(TunerSettings::OnFMChanged), NULL, this);

}

void TunerSettings::OnAMChanged(wxCommandEvent& event) {
	auto commandEvent = std::make_unique<wxCommandEvent>(wxEVT_MEDIAPLAYER_DIALOG_EVENT, MEDIAPLAYER_DELETE_ALLPRESET);
	commandEvent->SetInt(0); // Arbitrarily use 0 to denote AM, 1 for FM
	SubmitEvent(std::move(commandEvent));
}

void TunerSettings::OnFMChanged(wxCommandEvent& event) {
	auto commandEvent = std::make_unique<wxCommandEvent>(wxEVT_MEDIAPLAYER_DIALOG_EVENT, MEDIAPLAYER_DELETE_ALLPRESET);
	commandEvent->SetInt(1); 
	SubmitEvent(std::move(commandEvent));
}


void TunerSettings::OnRegionChanged(wxCommandEvent& event) {
	//std::queue<std::unique_ptr<wxEvent>> jobQueue;
	
	//wxCommandEvent commandEvent(wxEVT_MEDIAPLAYER_DIALOG_EVENT, MEDIAPLAYER_SET_TUNER_REGION);
	//commandEvent.SetInt(choiceRegion->GetSelection());
	//jobQueue.push(std::unique_ptr<wxEvent>(commandEvent.Clone()));

	// Reverse
	//std::unique_ptr<wxEvent> event = std::move(jobQueue.front());
	//jobQueue.pop();
	//wxCommandEvent* commandEvent = dynamic_cast<wxCommandEvent*>(event.get());

	auto commandEvent = std::make_unique<wxCommandEvent>(wxEVT_MEDIAPLAYER_DIALOG_EVENT, MEDIAPLAYER_SET_TUNER_REGION);
	commandEvent->SetInt(choiceRegion->GetSelection());
	SubmitEvent(std::move(commandEvent));
	

}

void TunerSettings::OnStepChanged(wxCommandEvent& event) {
	auto commandEvent = std::make_unique<wxCommandEvent>(wxEVT_MEDIAPLAYER_DIALOG_EVENT, MEDIAPLAYER_SET_TUNER_STEP);
	commandEvent->SetInt(choiceStep->GetSelection());
	SubmitEvent(std::move(commandEvent));
}

// Device Settings
// Not much here, Set the device name...
// Could add static information (software version), factory reset etc.

const std::vector<wxString>& DeviceSettings::SearchTerms() {
	static const std::vector<wxString> terms = {
		_("Device Settings"),	// [0] page title
		_("Name"),				// [1]
		_("Device")				// [2]
	};
	return terms;
}

DeviceSettings::DeviceSettings(wxScrolledWindow* parent, wxFlexGridSizer* sizer, wxString name, EventSubmitter submitter) : SettingsTemplate(parent, sizer, name, submitter) {

	labelDeviceName = new wxStaticText(parent, wxID_ANY, _("Device Name"), wxDefaultPosition, wxDefaultSize, 0);
	labelDeviceName->Wrap(-1);
	sizer->Add(labelDeviceName, 0, wxEXPAND, 0);

	textDeviceName = new wxTextCtrl(parent, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0);
	sizer->Add(textDeviceName, 0, wxEXPAND, 0);
	textDeviceName->SetValue(mediaPlayer.deviceName);

	// Connect the event handler
	textDeviceName->Connect(wxEVT_TEXT, wxCommandEventHandler(DeviceSettings::OnDeviceNameChanged), NULL, this);

	// Associate controls with the search terms
	const std::vector<wxString>& t = SearchTerms();
	Register(t[1], labelDeviceName, textDeviceName);
	Register(t[2], labelDeviceName, textDeviceName);
}

DeviceSettings::~DeviceSettings() {
	textDeviceName->Disconnect(wxEVT_TEXT, wxCommandEventHandler(DeviceSettings::OnDeviceNameChanged), NULL, this);
}

void DeviceSettings::OnDeviceNameChanged(wxCommandEvent& event) {
	auto commandEvent = std::make_unique<wxCommandEvent>(wxEVT_MEDIAPLAYER_DIALOG_EVENT, MEDIAPLAYER_SET_DEVICE_NAME	);
	commandEvent->SetString(textDeviceName->GetValue());
	SubmitEvent(std::move(commandEvent));
}

// All Zone Settings
// Top level page for zones, beneath are pages for Zones 0, 1 and 2

const std::vector<wxString>& AllZoneSettings::SearchTerms() {
	static const std::vector<wxString> terms = {
		_("Zones"),		// [0] page title
		_("Link")		// [1]
	};
	return terms;
}

AllZoneSettings::AllZoneSettings(wxScrolledWindow* parent, wxFlexGridSizer* sizer, wxString name, EventSubmitter submitter) : SettingsTemplate(parent, sizer, name, submitter) {

	checkLink = new wxCheckBox(parent, wxID_ANY, "Link to zone", wxDefaultPosition, wxDefaultSize);
	sizer->Add(checkLink, 0, wxEXPAND, 0);

	// Connect the event handlers
	checkLink->Connect(wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler(AllZoneSettings::OnLinkChanged), NULL, this);

	// Associate controls with the search terms
	const std::vector<wxString>& t = SearchTerms();
	Register(t[1], checkLink);
}

AllZoneSettings::~AllZoneSettings() {
	checkLink->Disconnect(wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler(AllZoneSettings::OnLinkChanged), NULL, this);
}

void AllZoneSettings::OnLinkChanged(wxCommandEvent& event) {
	auto commandEvent = std::make_unique<wxCommandEvent>(wxEVT_MEDIAPLAYER_DIALOG_EVENT, MEDIAPLAYER_JOB_APPEND_TEXT);
	commandEvent->SetInt(checkLink->GetValue());
	SubmitEvent(std::move(commandEvent));
}

const std::vector<wxString>& ZoneSettings::SearchTerms() {
	static const std::vector<wxString> terms = {
		_("Zone Settings"),	// [0] page title
		_("Name"),			// [1]
		_("Zone"),			// [2]
		_("Volume"),		// [3]
		_("Limit")			// [4]
	};
	return terms;
}

ZoneSettings::ZoneSettings(wxScrolledWindow* parent, wxFlexGridSizer* sizer, wxString name, EventSubmitter submitter) : SettingsTemplate(parent, sizer, name, submitter) {

	labelZoneName = new wxStaticText(parent, wxID_ANY, _("Zone Name"), wxDefaultPosition, wxDefaultSize, 0);
	labelZoneName->Wrap(-1);
	sizer->Add(labelZoneName, 0, wxEXPAND, 0);

	textZoneName = new wxTextCtrl(parent, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0);
	sizer->Add(textZoneName, 0, wxEXPAND, 0);

	labelVolumeLimit = new wxStaticText(parent, wxID_ANY, _("Volume Limit"), wxDefaultPosition, wxDefaultSize, 0);
	labelVolumeLimit->Wrap(-1);
	sizer->Add(labelVolumeLimit, 0, wxEXPAND, 0);

	sliderVolumeLimit = new wxSlider(parent, wxID_ANY, 1, 0, 15, wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL);
	sizer->Add(sliderVolumeLimit, 1, wxALL, 5);

	// Connect the event handlers
	textZoneName->Connect(wxEVT_TEXT, wxCommandEventHandler(ZoneSettings::OnZoneNameChanged), NULL, this);
	sliderVolumeLimit->Connect(wxEVT_TEXT, wxCommandEventHandler(ZoneSettings::OnVolumeLimitChanged), NULL, this);


	// Associate controls with the search terms
	const std::vector<wxString>& t = SearchTerms();
	Register(t[1], labelZoneName, textZoneName);
	Register(t[2], labelZoneName, textZoneName);
	Register(t[3], labelVolumeLimit, sliderVolumeLimit);
	Register(t[4], labelVolumeLimit, sliderVolumeLimit);

	// Initialize the values
	zoneNumber = std::atoi(name);
	switch (zoneNumber) {
		case 0:
			textZoneName->SetValue(mediaPlayer.zoneOneName);
			sliderVolumeLimit->SetValue(mediaPlayer.zoneOneVolumeLimit);
			break;
		case 1:
			textZoneName->SetValue(mediaPlayer.zoneTwoName);
			sliderVolumeLimit->SetValue(mediaPlayer.zoneTwoVolumeLimit);
			break;
		case 2:
			textZoneName->SetValue(mediaPlayer.zoneThreeName);
			sliderVolumeLimit->SetValue(mediaPlayer.zoneThreeVolumeLimit);
			break;
	}
}

ZoneSettings::~ZoneSettings() {
	textZoneName->Disconnect(wxEVT_TEXT, wxCommandEventHandler(ZoneSettings::OnZoneNameChanged), NULL, this);
	sliderVolumeLimit->Disconnect(wxEVT_TEXT, wxCommandEventHandler(ZoneSettings::OnVolumeLimitChanged), NULL, this);
}

void ZoneSettings::OnZoneNameChanged(wxCommandEvent& event) {
	auto commandEvent = std::make_unique<wxCommandEvent>(wxEVT_MEDIAPLAYER_DIALOG_EVENT, MEDIAPLAYER_SET_ZONE_NAME);
	commandEvent->SetInt(zoneNumber);
	commandEvent->SetString(textZoneName->GetValue());
	SubmitEvent(std::move(commandEvent));
}

void ZoneSettings::OnVolumeLimitChanged(wxCommandEvent& event) {
	auto commandEvent = std::make_unique<wxCommandEvent>(wxEVT_MEDIAPLAYER_DIALOG_EVENT, MEDIAPLAYER_SET_VOLUME_LIMIT);
	commandEvent->SetInt(zoneNumber);
	commandEvent->SetString(wxString::Format("%d", sliderVolumeLimit->GetValue()));
	SubmitEvent(std::move(commandEvent));
}

