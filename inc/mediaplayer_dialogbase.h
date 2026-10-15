///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#pragma once

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/sizer.h>
#include <wx/gauge.h>
#include <wx/statbox.h>
#include <wx/listbox.h>
#include <wx/button.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/radiobox.h>
#include <wx/checkbox.h>
#include <wx/panel.h>
#include <wx/slider.h>
#include <wx/notebook.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class MediaPlayerDialogBase
///////////////////////////////////////////////////////////////////////////////
class MediaPlayerDialogBase : public wxPanel
{
	private:

	protected:
		wxNotebook* mediaPlayerNotebook;
		wxPanel* panelMedia;
		wxStaticBoxSizer* sizerStatus;
		wxStaticText* labelMediaPlayerStatus;
		wxGauge* gaugeProgress;
		wxStaticBoxSizer* sizerMedia;
		wxListBox* listFiles;
		wxButton* buttonCurrent;
		wxButton* buttonChild;
		wxButton* buttonParent;
		wxRadioBox* radioBoxSource;
		wxCheckBox* checkShuffle;
		wxCheckBox* checkRepeat;
		wxPanel* panelVolume;
		wxStaticBoxSizer* sizerZone0Volume;
		wxSlider* sliderZone0Volume;
		wxStaticBoxSizer* sizerZone1Volume;
		wxSlider* sliderZone1Volume;
		wxSlider* sliderBalance;
		wxSlider* sliderTreble;
		wxSlider* sliderMidrange;
		wxSlider* sliderBass;
		wxButton* buttonReverse;
		wxButton* buttonPause;
		wxButton* buttonForward;
		wxButton* buttonMute;
		wxButton* buttonPower;

		// Virtual event handlers, override them in your derived class
		virtual void OnInit( wxInitDialogEvent& event ) { event.Skip(); }
		virtual void OnFileSelected( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnButtonCurrent( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnButtonChild( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnButtonParent( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnSourceChanged( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnCheckShuffle( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnCheckRepeat( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnZone0VolumeChanged( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnZone1VolumeChanged( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnBalanceChanged( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnTrebleChanged( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnMidrangeChanged( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnBassChanged( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnButtonReverse( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnButtonPause( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnButtonForward( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnButtonMute( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnButtonPower( wxCommandEvent& event ) { event.Skip(); }


	public:

		MediaPlayerDialogBase( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 575,548 ), long style = wxTAB_TRAVERSAL, const wxString& name = wxEmptyString );

		~MediaPlayerDialogBase();

};

