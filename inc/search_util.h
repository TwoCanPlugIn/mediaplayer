// Shared matching rule used by both the tree filter and the control highlighting,
// so the two can never disagree.
#pragma once

#include <wx/wx.h>
#include <wx/arrstr.h>

// True if every space-separated word in 'filter' appears in 'text' (case-insensitive).
// An empty filter matches everything.
inline bool MatchesFilter(const wxString& text, const wxString& filter) {
	if (filter.IsEmpty()) return true;

	const wxString haystack = text.Lower();
	const wxArrayString tokens = wxSplit(filter.Lower(), ' ');

	for (size_t i = 0; i < tokens.GetCount(); i++) {
		if (!tokens[i].IsEmpty() && !haystack.Contains(tokens[i])) return false;
	}
	return true;
}
