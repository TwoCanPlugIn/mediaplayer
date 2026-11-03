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
// along with the Fusion Media Player plugin for OpenCPN. If not, see <https://www.gnu.org/licenses/>.
//


// Project: Fusion Media Player Plugin
// Description: Visual Studio like settings dialog for Fusion Media Player plugin 
// Owner: twocanplugin@hotmail.com
// Date: 6/1/2020
// Version History: 
// 1.0 Initial Release
// 1.1 Search capabilities
//

// Readme for how to make additions
/*
Adding a new page touches several places: 
PageId enum, 
GetSettingsIndex() - > the index builder
BuildTree (the match, show and select logic), 
LoadPage. 
If you expect many pages, a small table would replace most of that. 
Each row would hold the page ID, its title, its parent, a factory function and a 
SearchTerms() function. BuildTree could then loop over the table, 
and parent-only handling would come from the parent field rather than the 
hardcoded Network-under-Basic rule

To add a setting to an existing page: 
Add its string to that page’s SearchTerms()
Create the control
Call Register(t[n], control). 
The t[n] indexes are positional.
Keep the title at [0] and don’t reorder the list without checking the Register calls.
The _() strings in SearchTerms() are translated, 
so users searching in their own language match the translated text. 
*/
// BUG BUG Enhancement
/*
// in the header
enum TunerTerm { TUNER_TITLE, TUNER_REGION, TUNER_USA, TUNER_EUROPE, TUNER_JAPAN,
				 TUNER_AUSTRALASIA, TUNER_STEP, TUNER_AUTO, TUNER_MANUAL };

// in the constructor
Register(t[TUNER_REGION], labelRegion, choiceRegion);
Register(t[TUNER_STEP],   labelStep,   choiceStep);
Register(t[TUNER_AUTO],   checkAuto);

Also to prefill for example the Zone Name and the tree
// SettingsTemplate (header)
protected:
	wxString pageName;

// SettingsTemplate constructor
SettingsTemplate::SettingsTemplate(wxScrolledWindow* parent, wxFlexGridSizer* sizer, wxString name)
	: pageName(name) {
	parentWindow = parent;
	panelSizer = sizer;
	sizer->SetVGap(10);
}

// Consider templates and a factory

struct PageDescriptor {
	PageId id;
	PageId parent;                                   // PAGE_NONE for top level
	const std::vector<wxString>& (*searchTerms)();   // [0] is the title
	std::function<SettingsTemplate*(wxScrolledWindow*, wxFlexGridSizer*, const wxString&)> create;
};

const std::vector<PageDescriptor>& GetPages() {
	static const std::vector<PageDescriptor> pages = {
		{ PAGE_DEVICE, PAGE_NONE,  &DeviceSettings::SearchTerms,  &Make<DeviceSettings>  },
		{ PAGE_ZONES,  PAGE_NONE,  &AllZoneSettings::SearchTerms, &Make<AllZoneSettings> },
		{ PAGE_ZONE0,  PAGE_ZONES, &ZoneSettings::SearchTerms,    &Make<ZoneSettings>    },
		{ PAGE_ZONE1,  PAGE_ZONES, &ZoneSettings::SearchTerms,    &Make<ZoneSettings>    },
		{ PAGE_ZONE2,  PAGE_ZONES, &ZoneSettings::SearchTerms,    &Make<ZoneSettings>    },
		{ PAGE_TUNER,  PAGE_NONE,  &TunerSettings::SearchTerms,   &Make<TunerSettings>   },
	};
	return pages;
}

// This is the only "template" you need: a tiny factory function
template <typename T>
SettingsTemplate* Make(wxScrolledWindow* p, wxFlexGridSizer* s, const wxString& n) {
	return new T(p, s, n);
}

To implement

Index: GetSettingsIndex() becomes one loop over GetPages(), with no per-page copy.
PageMatches: use the index as it is.
LoadPage: the switch becomes settingsTemplate = FindPage(page).create(scrollWindow, sizerFlex, name);
BuildTree: loop the table, append top-level pages first, then children under their parent’s wxTreeItemId. Keep a std::map<PageId, wxTreeItemId> so selecting a target is a lookup rather than a switch.
Parent-only rule: show a page if it matches itself or any child matches, derived from the parent field rather than the hardcoded zone rule.
visibleMask: replace it with a std::set<PageId> of visible pages, or a std::bitset that scales past 32 pages

And for the search terms...

enum TunerTerm { TUNER_TITLE, TUNER_REGION, TUNER_STEP, TUNER_AUTO, TUNER_COUNT };

static const wxString& Term(TunerTerm t) {
	static const wxString text[TUNER_COUNT] = {
		_("Tuner"), _("Region"), _("Step"), _("Automatic")
	};
	return text[t];
}
*/

#include "mediaplayer_options.h"

// Searches through the indexes for each option page
#include "search_util.h"

// STL
#include <vector>
#include <queue>

// wxWidgets Search Control
#include <wx/srchctrl.h>

namespace {

	// The page and it's keywords.
	struct SettingEntry {
		PageId page;
		wxString text;
	};

	// Initialize indexes containing each option page's search terms
	const std::vector<SettingEntry>& GetSettingsIndex() {
		static std::vector<SettingEntry> index;
		if (index.empty()) {
			for (const wxString& s : DeviceSettings::SearchTerms()) {
				index.push_back({ PAGE_DEVICE, s });
			}
			for (const wxString& s : AllZoneSettings::SearchTerms()) {
				index.push_back({ PAGE_ZONES, s });
			}
			for (const wxString& s : TunerSettings::SearchTerms()) {
				index.push_back({ PAGE_TUNER, s });
			}
			for (const wxString& s : ZoneSettings::SearchTerms()) {
				index.push_back({ PAGE_ZONE0, s });
				index.push_back({ PAGE_ZONE1, s });
				index.push_back({ PAGE_ZONE2, s });
			}
		}
		return index;
	}
}
// The settings dialog holding the options pages

// Constructor and destructor implementation
MediaPlayerOptions::MediaPlayerOptions(wxWindow* parent, wxWindowID id, const wxString& title, const wxSize& size, wxEvtHandler* eventHandler)
	: MediaPlayerOptionsBase(parent, id, title, wxDefaultPosition, size, wxCAPTION | wxCLOSE_BOX | wxDEFAULT_DIALOG_STYLE | wxMINIMIZE_BOX | wxSYSTEM_MENU),
	eventTarget(eventHandler)
	{}

MediaPlayerOptions::~MediaPlayerOptions() {
	// Covers the case where the dialog is closed with the title bar X rather than OK/Cancel.
	// Child controls are still alive here, so the page can safely Disconnect its handlers.
	ClearPage();
	EndModal(wxID_CANCEL);
}


void MediaPlayerOptions::OnInit(wxInitDialogEvent& event) {

	// The generated UI hides the cancel (clear) button; show it so the filter can be cleared
	searchOptions->ShowCancelButton(true);

	// wxWidgets Search control. Search whie typing & Cancel
	searchOptions->Bind(wxEVT_TEXT, &MediaPlayerOptions::OnSearchText, this);
	searchOptions->Bind(wxEVT_SEARCH_CANCEL, &MediaPlayerOptions::OnSearchCancel, this);

	// Build the full tree and default to the first page
	currentPage = PAGE_DEVICE;
	BuildTree(wxEmptyString);

	// I hate that OpenCPN dialogs don't default to the enter key!
	// Hitting the enter key on anything but a text control should default to the OK button
	sizerButtonsOK->SetFocus();

}

void MediaPlayerOptions::OnOk(wxCommandEvent& event) {
	// Dequeue any updates
	while (!eventQueue.empty())	{
		auto queuedEvent = std::move(eventQueue.front());
		eventQueue.pop();
		wxQueueEvent(eventTarget, queuedEvent.release());
	}
	ClearPage();
	EndModal(wxID_OK);
}

void MediaPlayerOptions::OnCancel(wxCommandEvent& event) {
	ClearPage();
	EndModal(wxID_CANCEL);
}

void MediaPlayerOptions::OnChar(wxKeyEvent& event) {
	wxMessageBox(wxString::Format("Key Code: %d", event.GetKeyCode()));
}

// Search button clicked, the filter is already applied live as the user types,
// so this just re-applies whatever is in the box
void MediaPlayerOptions::OnSearch(wxCommandEvent& event) {
	BuildTree(searchOptions->GetValue().Trim().Trim(false));
}

// Helper function to populate the zone names in the options pages
wxString MediaPlayerOptions::GetZoneName(int zone) const {
	wxString name;
	switch (zone) {
		case 0:
			name = mediaPlayer.zoneOneName;
		break;
		case 1:
			name = mediaPlayer.zoneTwoName;
		break;
		case 2:
			name = mediaPlayer.zoneThreeName;
		break;
	}
	if (name.IsEmpty()) {
		name = wxString::Format(_("Zone %d"), zone);   // Fallback if no zone names
	}
	return name;
}

// Perform the search and only display the tree items whose controls match the search term
void MediaPlayerOptions::OnSearchText(wxCommandEvent& event) {
	// Rebuilding the tree and page takes focus away from the search box. Put it back exactly
	// as the user left it (focus and caret/selection), otherwise typing is interrupted
	// and a bare SetFocus() would select all the text.
	long from = 0, to = 0;
	searchOptions->GetSelection(&from, &to);

	BuildTree(event.GetString().Trim().Trim(false));

	searchOptions->SetFocus();
	searchOptions->SetSelection(from, to);
}

// If the user hits the search cancel button
void MediaPlayerOptions::OnSearchCancel(wxCommandEvent& event) {
	// BUG BUG This may or may not raise wxEVT_TEXT depending on platform
	searchOptions->Clear();   
	BuildTree(wxEmptyString);
	searchOptions->SetFocus();
}


// Tree and Page management

// If we've found a settings page that matched the search options
bool MediaPlayerOptions::PageMatches(PageId page, const wxString& filter) {
	if (filter.IsEmpty()) {
		return true;
	}

	for (const SettingEntry& e : GetSettingsIndex()) {
		if (e.page == page && MatchesFilter(e.text, filter)) {
			return true;
		}
	}
	return false;
}

// Iterate over the pages that matched the search options and rebuild the tree
void MediaPlayerOptions::BuildTree(const wxString& filter) {
	// must be set before LoadPage()
	currentFilter = filter;   

	// Which pages contain a matching setting
	const bool matchDevice = PageMatches(PAGE_DEVICE, filter);
	const bool matchTuner = PageMatches(PAGE_TUNER, filter);
	const bool matchZones = PageMatches(PAGE_ZONES, filter);
	const bool matchZone0 = PageMatches(PAGE_ZONE0, filter);
	const bool matchZone1 = PageMatches(PAGE_ZONE1, filter);
	const bool matchZone2 = PageMatches(PAGE_ZONE2, filter);

	// Which tree items to show. 
	// zone0, 1 and 2 are children of match Zones
	const bool showZones = matchZones || matchZone0 || matchZone1 || matchZone2;
	const bool showDevice = matchDevice; 
	const bool showTuner = matchTuner;

	// Same pages matching as last time. Leave the tree alone and just re-apply the filter.
	// (rebuildingTree is deliberately not touched on this path.)
	const int mask = (matchDevice ? 1 : 0) | (matchTuner ? 2 : 0) | (matchZones ? 4 : 0) |
		(matchZone0 ? 8 : 0) | (matchZone1 ? 16 : 0) | (matchZone2 ? 32 : 0);
	if (mask == visibleMask && settingsTemplate) {
		settingsTemplate->ApplyFilter(filter);
		return;
	}
	visibleMask = mask;

	// Rebuild the tree
	rebuildingTree = true;
	treeOptions->Freeze();

	ClearPage();
	treeOptions->DeleteAllItems();

	wxTreeItemId rootItem = treeOptions->AddRoot("Dummy");
	wxTreeItemId deviceItem, zoneItem, tunerItem, zone0Item, zone1Item, zone2Item;

	if (showDevice) {
		deviceItem = treeOptions->AppendItem(rootItem, DeviceSettings::SearchTerms()[0], -1, -1, new PageData(PAGE_DEVICE));
	}
	if (showZones) {
		zoneItem = treeOptions->AppendItem(rootItem, AllZoneSettings::SearchTerms()[0], -1, -1, new PageData(PAGE_ZONES));
		zone0Item = treeOptions->AppendItem(zoneItem, GetZoneName(0), -1, -1, new PageData(PAGE_ZONE0));
		zone1Item = treeOptions->AppendItem(zoneItem, GetZoneName(1), -1, -1, new PageData(PAGE_ZONE1));
		zone2Item = treeOptions->AppendItem(zoneItem, GetZoneName(2), -1, -1, new PageData(PAGE_ZONE2));
		treeOptions->Expand(zoneItem);
	}
	
	if (showTuner) {
		tunerItem = treeOptions->AppendItem(rootItem, TunerSettings::SearchTerms()[0], -1, -1, new PageData(PAGE_TUNER));
	}

	treeOptions->Thaw();

	// Choose the page to select. Only pages that match are displayed
	// A parent that is only shown to hold a matching child is not.
	// Stay on the current page if it matches; otherwise take the first match in tree order.
	const bool currentMatches =
		(currentPage == PAGE_DEVICE && matchDevice) ||
		(currentPage == PAGE_TUNER && matchTuner) ||
		(currentPage == PAGE_ZONES && matchZones) ||
		(currentPage == PAGE_ZONE0 && matchZone0) ||
		(currentPage == PAGE_ZONE1 && matchZone1) ||
		(currentPage == PAGE_ZONE2 && matchZone2);

	PageId pick = currentPage;
	if (!currentMatches) {
		if (!currentMatches) {
			if (matchDevice)      pick = PAGE_DEVICE;
			else if (matchZones)  pick = PAGE_ZONES;
			else if (matchZone0)  pick = PAGE_ZONE0;
			else if (matchZone1)  pick = PAGE_ZONE1;
			else if (matchZone2)  pick = PAGE_ZONE2;
			else                  pick = PAGE_TUNER;
		}
	}

	// If nothing matched at all, none of the items exist and target stays invalid
	wxTreeItemId target;
	switch (pick) {
	case PAGE_DEVICE:
		target = deviceItem; 
	break;
	case PAGE_TUNER: 
		target = tunerItem; 
	break;
	case PAGE_ZONES:
		target = zoneItem;
	break;
	case PAGE_ZONE0:
		target = zone0Item;
		break;
	case PAGE_ZONE1:
		target = zone1Item;
		break;
	case PAGE_ZONE2:
		target = zone2Item;
		break;
	}

	if (target.IsOk()) {
		treeOptions->SelectItem(target, true);
		treeOptions->EnsureVisible(target);
	}

	// Flag stays set until after SelectItem so its event doesn't load the page a second time
	rebuildingTree = false;

	if (target.IsOk()) {
		PageData* data = static_cast<PageData*>(treeOptions->GetItemData(target));
		if (data) {
			LoadPage(data->pageId);
		}
	}
	else {
		// No results for search items
		ShowNoResults();
	}
}

// Deletes the current page object while its controls are still alive 
// so Disconnect is safe, and then destroys the controls.
void MediaPlayerOptions::ClearPage() {
	//delete settingsTemplate;
	settingsTemplate = nullptr;
	sizerFlex->Clear(true);
}

void MediaPlayerOptions::ShowNoResults() {
	wxStaticText* message = new wxStaticText(scrollWindow, wxID_ANY, _("No matching settings"));
	sizerFlex->Add(message, 0, wxALL, 10);
	scrollWindow->FitInside();
	Layout();
}

void MediaPlayerOptions::LoadPage(PageId page) {
	ClearPage();
	scrollWindow->Scroll(0, 0);   // forget any offset left over from a previous search
	currentPage = page;

	switch (page) {
	case PAGE_ZONES:
		//settingsTemplate = new AllZoneSettings(scrollWindow, sizerFlex, "Zones");
		settingsTemplate = std::make_unique<AllZoneSettings>(scrollWindow, sizerFlex, "Zones", GetEventSubmitter());
		break;
	case PAGE_ZONE0:
		//settingsTemplate = new ZoneSettings(scrollWindow, sizerFlex, GetZoneName(0));
		settingsTemplate = std::make_unique<ZoneSettings>(scrollWindow, sizerFlex, "0", GetEventSubmitter());
		break;
	case PAGE_ZONE1:
		//settingsTemplate = new ZoneSettings(scrollWindow, sizerFlex, GetZoneName(1));
		settingsTemplate = std::make_unique<ZoneSettings>(scrollWindow, sizerFlex, "1", GetEventSubmitter());
		break;
	case PAGE_ZONE2:
		//settingsTemplate = new ZoneSettings(scrollWindow, sizerFlex, GetZoneName(2));
		settingsTemplate = std::make_unique<ZoneSettings>(scrollWindow, sizerFlex, "2", GetEventSubmitter());
		break;
	case PAGE_TUNER:
		//settingsTemplate = new TunerSettings(scrollWindow, sizerFlex, "Tuner");
		settingsTemplate = std::make_unique<TunerSettings>(scrollWindow, sizerFlex, "Tuner", GetEventSubmitter());
		break;
	case PAGE_DEVICE:
		//settingsTemplate = new DeviceSettings(scrollWindow, sizerFlex, "Device");
		settingsTemplate = std::make_unique<DeviceSettings>(scrollWindow, sizerFlex, "Device", GetEventSubmitter());
		break;
	}

	// Lay the new controls out first so positions are valid when we scroll to the first match
	scrollWindow->FitInside();
	Layout();

	if (settingsTemplate) {
		settingsTemplate->ApplyFilter(currentFilter);
	}
}

void MediaPlayerOptions::OnTreeChanging(wxTreeEvent& event) {
	// Nothing to do as LoadPage() clears the previous page
}

void MediaPlayerOptions::OnTreeSelected(wxTreeEvent& event) {
	// Ignore events caused by BuildTree() rebuilding the tree
	if (rebuildingTree || !event.GetItem().IsOk()) {
		return;
	}

	PageData* data = static_cast<PageData*>(treeOptions->GetItemData(event.GetItem()));
	if (data) {
		LoadPage(data->pageId);
	}
}
