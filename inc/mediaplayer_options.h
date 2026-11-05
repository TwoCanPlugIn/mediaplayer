// Copyright(C) 2018-2020 by Steven Adler
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

#ifndef MEDIAPLAYER_OPTIONS_H
#define MEDIAPLAYER_OPTIONS_H

// wxWidgets Precompiled Headers
#include "wx/wxprec.h"

#ifndef  WX_PRECOMP
#include "wx/wx.h"
#endif

#include <wx/treectrl.h>

// The dialog base class from which we are derived
// Note wxFormBuilder was used to generate the UI
#include "mediaplayer_optionsbase.h"

// Panels for each set of options
#include "mediaplayer_optionpages.h"


extern FUSION_MEDIA_PLAYER mediaPlayer;

// Id's associated with searching
enum PageId { PAGE_ABOUT, PAGE_DEVICE, PAGE_ZONES, PAGE_TUNER, PAGE_ZONE0, PAGE_ZONE1, PAGE_ZONE2 };

// Attached to each tree item so a selection can be mapped back to a page,
// (tree item IDs change every time the tree is rebuilt by a search)
class PageData : public wxTreeItemData {
public:
	PageData(PageId id) : pageId(id) {}
	PageId pageId;
};

// Settings Base class created by wxFormBuilder
// All the UI base classes (Remote Control and  Options dialogs) are in a single source file
class MediaPlayerOptions : public MediaPlayerOptionsBase {
	
public:
	MediaPlayerOptions(wxWindow* parent, wxWindowID id,const wxString& title , const wxSize& size, wxEvtHandler* eventHandler);
	~MediaPlayerOptions();

	using EventSubmitter =	std::function<void(std::unique_ptr<wxEvent>)>;

	EventSubmitter GetEventSubmitter() {
		return [this](std::unique_ptr<wxEvent> event) {
			eventQueue.push(std::move(event));
			};
	}


protected:
	// Overridden methods from the base class
	void OnInit(wxInitDialogEvent& event) override;
	void OnOk(wxCommandEvent& event) override;
	void OnCancel(wxCommandEvent& event) override;
	void OnTreeSelected(wxTreeEvent& event) override;
	void OnTreeChanging(wxTreeEvent& event) override;
	void OnSearch(wxCommandEvent& event) override;

private:
	
	// Base class for the option pagess
	std::unique_ptr<SettingsTemplate> settingsTemplate;

	// Search related stuff
	// Populate the Zone Names
	wxString GetZoneName(int zone) const;

	// Build the Tree Control
	void BuildTree(const wxString& filter);

	// Manage each option page/panel
	void LoadPage(PageId page);
	void ClearPage();

	// If no results returned for the search terms
	void ShowNoResults();

	// Find pages/panels that match the search terms
	static bool PageMatches(PageId page, const wxString& filter);

	// Event Handlers for the search control
	void OnSearchText(wxCommandEvent& event);
	void OnSearchCancel(wxCommandEvent& event);

	wxString currentFilter;
	PageId currentPage = PAGE_DEVICE;
	bool rebuildingTree = false;
	int visibleMask = -1; 
	
	// Event Queue and handling
	wxEvtHandler* eventTarget;
	std::queue<std::unique_ptr<wxEvent>> eventQueue;

};

#endif
