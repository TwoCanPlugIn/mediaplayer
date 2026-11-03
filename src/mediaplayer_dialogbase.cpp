///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "mediaplayer_dialogbase.h"

///////////////////////////////////////////////////////////////////////////

MediaPlayerDialogBase::MediaPlayerDialogBase( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : wxPanel( parent, id, pos, size, style, name )
{
	wxBoxSizer* sizerDialog;
	sizerDialog = new wxBoxSizer( wxVERTICAL );

	panelMedia = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* sizerPanel;
	sizerPanel = new wxBoxSizer( wxVERTICAL );

	sizerStatus = new wxStaticBoxSizer( new wxStaticBox( panelMedia, wxID_ANY, wxT("Media Player Status") ), wxVERTICAL );

	wxBoxSizer* sizerLabel;
	sizerLabel = new wxBoxSizer( wxHORIZONTAL );

	labelMediaPlayerStatus = new wxStaticText( sizerStatus->GetStaticBox(), wxID_ANY, wxT("Now playing..."), wxDefaultPosition, wxDefaultSize, 0 );
	labelMediaPlayerStatus->Wrap( -1 );
	sizerLabel->Add( labelMediaPlayerStatus, 1, wxALL, 5 );


	sizerStatus->Add( sizerLabel, 0, wxEXPAND, 5 );

	wxBoxSizer* sizerGauge;
	sizerGauge = new wxBoxSizer( wxHORIZONTAL );

	gaugeProgress = new wxGauge( sizerStatus->GetStaticBox(), wxID_ANY, 100, wxDefaultPosition, wxDefaultSize, wxGA_HORIZONTAL );
	gaugeProgress->SetValue( 0 );
	sizerGauge->Add( gaugeProgress, 1, wxALL, 5 );


	sizerStatus->Add( sizerGauge, 0, wxEXPAND, 5 );


	sizerPanel->Add( sizerStatus, 0, wxEXPAND, 5 );

	sizerMedia = new wxStaticBoxSizer( new wxStaticBox( panelMedia, wxID_ANY, wxT("Media") ), wxHORIZONTAL );

	wxBoxSizer* sizerFiles;
	sizerFiles = new wxBoxSizer( wxVERTICAL );

	wxArrayString choiceSourceChoices;
	choiceSource = new wxChoice( sizerMedia->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, choiceSourceChoices, wxCB_SORT );
	choiceSource->SetSelection( 0 );
	sizerFiles->Add( choiceSource, 0, wxALL|wxEXPAND, 5 );

	listFiles = new wxListBox( sizerMedia->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, 0, NULL, 0 );
	sizerFiles->Add( listFiles, 1, wxALL|wxEXPAND, 5 );

	wxBoxSizer* sizerDebug;
	sizerDebug = new wxBoxSizer( wxHORIZONTAL );


	sizerFiles->Add( sizerDebug, 0, wxEXPAND, 5 );


	sizerMedia->Add( sizerFiles, 1, wxEXPAND, 5 );

	wxBoxSizer* sizerControls;
	sizerControls = new wxBoxSizer( wxVERTICAL );

	sizerZone0Volume = new wxStaticBoxSizer( new wxStaticBox( sizerMedia->GetStaticBox(), wxID_ANY, wxT("Volume Zone 0") ), wxHORIZONTAL );

	sliderZone0Volume = new wxSlider( sizerZone0Volume->GetStaticBox(), wxID_ANY, 1, 0, 15, wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL );
	sizerZone0Volume->Add( sliderZone0Volume, 1, wxALL, 5 );


	sizerControls->Add( sizerZone0Volume, 1, wxEXPAND, 5 );

	sizerZone1Volume = new wxStaticBoxSizer( new wxStaticBox( sizerMedia->GetStaticBox(), wxID_ANY, wxT("Volume Zone 1") ), wxHORIZONTAL );

	sliderZone1Volume = new wxSlider( sizerZone1Volume->GetStaticBox(), wxID_ANY, 1, 0, 15, wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL );
	sizerZone1Volume->Add( sliderZone1Volume, 1, wxALL, 5 );


	sizerControls->Add( sizerZone1Volume, 1, wxEXPAND, 5 );

	wxStaticBoxSizer* sizerBalance;
	sizerBalance = new wxStaticBoxSizer( new wxStaticBox( sizerMedia->GetStaticBox(), wxID_ANY, wxT("Balance") ), wxHORIZONTAL );

	sliderBalance = new wxSlider( sizerBalance->GetStaticBox(), wxID_ANY, 0, -7, 7, wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL );
	sizerBalance->Add( sliderBalance, 1, wxALL, 5 );


	sizerControls->Add( sizerBalance, 1, wxEXPAND, 5 );

	wxStaticBoxSizer* sizeTreble;
	sizeTreble = new wxStaticBoxSizer( new wxStaticBox( sizerMedia->GetStaticBox(), wxID_ANY, wxT("Treble") ), wxHORIZONTAL );

	sliderTreble = new wxSlider( sizeTreble->GetStaticBox(), wxID_ANY, 0, -15, 15, wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL );
	sizeTreble->Add( sliderTreble, 1, wxALL, 5 );


	sizerControls->Add( sizeTreble, 1, wxEXPAND, 5 );

	wxStaticBoxSizer* sizerMidrange;
	sizerMidrange = new wxStaticBoxSizer( new wxStaticBox( sizerMedia->GetStaticBox(), wxID_ANY, wxT("Midrange") ), wxHORIZONTAL );

	sliderMidrange = new wxSlider( sizerMidrange->GetStaticBox(), wxID_ANY, 0, -15, 15, wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL );
	sizerMidrange->Add( sliderMidrange, 1, wxALL, 5 );


	sizerControls->Add( sizerMidrange, 1, wxEXPAND, 5 );

	wxStaticBoxSizer* sizerBass;
	sizerBass = new wxStaticBoxSizer( new wxStaticBox( sizerMedia->GetStaticBox(), wxID_ANY, wxT("Bass") ), wxHORIZONTAL );

	sliderBass = new wxSlider( sizerBass->GetStaticBox(), wxID_ANY, 0, -15, 15, wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL );
	sizerBass->Add( sliderBass, 1, wxALL, 5 );


	sizerControls->Add( sizerBass, 1, wxEXPAND, 5 );


	sizerMedia->Add( sizerControls, 1, wxEXPAND, 5 );


	sizerPanel->Add( sizerMedia, 1, wxEXPAND, 5 );


	panelMedia->SetSizer( sizerPanel );
	panelMedia->Layout();
	sizerPanel->Fit( panelMedia );
	sizerDialog->Add( panelMedia, 1, wxEXPAND | wxALL, 5 );

	wxBoxSizer* sizerButons;
	sizerButons = new wxBoxSizer( wxHORIZONTAL );

	buttonReverse = new wxButton( this, wxID_ANY, wxT("Previous"), wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT );
	sizerButons->Add( buttonReverse, 1, wxALL, 5 );

	buttonPause = new wxButton( this, wxID_ANY, wxT(" Play"), wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT );
	sizerButons->Add( buttonPause, 1, wxALL, 5 );

	buttonForward = new wxButton( this, wxID_ANY, wxT("Next"), wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT );
	sizerButons->Add( buttonForward, 1, wxALL, 5 );

	buttonMute = new wxButton( this, wxID_ANY, wxT("Mute"), wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT );
	sizerButons->Add( buttonMute, 1, wxALL, 5 );

	buttonPower = new wxButton( this, wxID_ANY, wxT("On"), wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT );
	sizerButons->Add( buttonPower, 1, wxALL, 5 );


	sizerDialog->Add( sizerButons, 0, wxEXPAND, 5 );


	this->SetSizer( sizerDialog );
	this->Layout();

	// Connect Events
	this->Connect( wxEVT_INIT_DIALOG, wxInitDialogEventHandler( MediaPlayerDialogBase::OnInit ) );
	choiceSource->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( MediaPlayerDialogBase::OnSourceChanged ), NULL, this );
	listFiles->Connect( wxEVT_COMMAND_LISTBOX_DOUBLECLICKED, wxCommandEventHandler( MediaPlayerDialogBase::OnFileSelected ), NULL, this );
	listFiles->Connect( wxEVT_RIGHT_UP, wxMouseEventHandler( MediaPlayerDialogBase::OnRightClick ), NULL, this );
	sliderZone0Volume->Connect( wxEVT_SLIDER, wxCommandEventHandler( MediaPlayerDialogBase::OnZone0VolumeChanged ), NULL, this );
	sliderZone1Volume->Connect( wxEVT_SLIDER, wxCommandEventHandler( MediaPlayerDialogBase::OnZone1VolumeChanged ), NULL, this );
	sliderBalance->Connect( wxEVT_SLIDER, wxCommandEventHandler( MediaPlayerDialogBase::OnBalanceChanged ), NULL, this );
	sliderTreble->Connect( wxEVT_SLIDER, wxCommandEventHandler( MediaPlayerDialogBase::OnTrebleChanged ), NULL, this );
	sliderMidrange->Connect( wxEVT_SLIDER, wxCommandEventHandler( MediaPlayerDialogBase::OnMidrangeChanged ), NULL, this );
	sliderBass->Connect( wxEVT_SLIDER, wxCommandEventHandler( MediaPlayerDialogBase::OnBassChanged ), NULL, this );
	buttonReverse->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( MediaPlayerDialogBase::OnButtonReverse ), NULL, this );
	buttonPause->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( MediaPlayerDialogBase::OnButtonPause ), NULL, this );
	buttonForward->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( MediaPlayerDialogBase::OnButtonForward ), NULL, this );
	buttonMute->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( MediaPlayerDialogBase::OnButtonMute ), NULL, this );
	buttonPower->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( MediaPlayerDialogBase::OnButtonPower ), NULL, this );
}

MediaPlayerDialogBase::~MediaPlayerDialogBase()
{
	// Disconnect Events
	this->Disconnect( wxEVT_INIT_DIALOG, wxInitDialogEventHandler( MediaPlayerDialogBase::OnInit ) );
	choiceSource->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( MediaPlayerDialogBase::OnSourceChanged ), NULL, this );
	listFiles->Disconnect( wxEVT_COMMAND_LISTBOX_DOUBLECLICKED, wxCommandEventHandler( MediaPlayerDialogBase::OnFileSelected ), NULL, this );
	listFiles->Disconnect( wxEVT_RIGHT_UP, wxMouseEventHandler( MediaPlayerDialogBase::OnRightClick ), NULL, this );
	sliderZone0Volume->Disconnect( wxEVT_SLIDER, wxCommandEventHandler( MediaPlayerDialogBase::OnZone0VolumeChanged ), NULL, this );
	sliderZone1Volume->Disconnect( wxEVT_SLIDER, wxCommandEventHandler( MediaPlayerDialogBase::OnZone1VolumeChanged ), NULL, this );
	sliderBalance->Disconnect( wxEVT_SLIDER, wxCommandEventHandler( MediaPlayerDialogBase::OnBalanceChanged ), NULL, this );
	sliderTreble->Disconnect( wxEVT_SLIDER, wxCommandEventHandler( MediaPlayerDialogBase::OnTrebleChanged ), NULL, this );
	sliderMidrange->Disconnect( wxEVT_SLIDER, wxCommandEventHandler( MediaPlayerDialogBase::OnMidrangeChanged ), NULL, this );
	sliderBass->Disconnect( wxEVT_SLIDER, wxCommandEventHandler( MediaPlayerDialogBase::OnBassChanged ), NULL, this );
	buttonReverse->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( MediaPlayerDialogBase::OnButtonReverse ), NULL, this );
	buttonPause->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( MediaPlayerDialogBase::OnButtonPause ), NULL, this );
	buttonForward->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( MediaPlayerDialogBase::OnButtonForward ), NULL, this );
	buttonMute->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( MediaPlayerDialogBase::OnButtonMute ), NULL, this );
	buttonPower->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( MediaPlayerDialogBase::OnButtonPower ), NULL, this );

}

MediaPlayerSettingsBase::MediaPlayerSettingsBase( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxDialog( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	wxBoxSizer* sizerDialog;
	sizerDialog = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* sizerExplorer;
	sizerExplorer = new wxBoxSizer( wxHORIZONTAL );

	wxBoxSizer* sizerNavigation;
	sizerNavigation = new wxBoxSizer( wxVERTICAL );

	searchOptions = new wxSearchCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	#ifndef __WXMAC__
	searchOptions->ShowSearchButton( true );
	#endif
	searchOptions->ShowCancelButton( false );
	sizerNavigation->Add( searchOptions, 0, wxALL|wxEXPAND, 5 );

	treeOptions = new wxTreeCtrl( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTR_DEFAULT_STYLE|wxTR_HIDE_ROOT );
	sizerNavigation->Add( treeOptions, 1, wxALL|wxEXPAND, 5 );


	sizerExplorer->Add( sizerNavigation, 1, wxEXPAND, 5 );

	wxBoxSizer* sizerScroll;
	sizerScroll = new wxBoxSizer( wxVERTICAL );

	scrollWindow = new wxScrolledWindow( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxHSCROLL|wxVSCROLL );
	scrollWindow->SetScrollRate( 5, 5 );
	sizerFlex = new wxFlexGridSizer( 0, 1, 0, 0 );
	sizerFlex->SetFlexibleDirection( wxVERTICAL );
	sizerFlex->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );


	scrollWindow->SetSizer( sizerFlex );
	scrollWindow->Layout();
	sizerFlex->Fit( scrollWindow );
	sizerScroll->Add( scrollWindow, 1, wxEXPAND | wxALL, 5 );


	sizerExplorer->Add( sizerScroll, 2, wxEXPAND, 5 );


	sizerDialog->Add( sizerExplorer, 1, wxEXPAND, 5 );

	sizerButtons = new wxStdDialogButtonSizer();
	sizerButtonsOK = new wxButton( this, wxID_OK );
	sizerButtons->AddButton( sizerButtonsOK );
	sizerButtonsCancel = new wxButton( this, wxID_CANCEL );
	sizerButtons->AddButton( sizerButtonsCancel );
	sizerButtons->Realize();

	sizerDialog->Add( sizerButtons, 0, wxALIGN_RIGHT|wxALL, 5 );


	this->SetSizer( sizerDialog );
	this->Layout();

	this->Centre( wxBOTH );

	// Connect Events
	searchOptions->Connect( wxEVT_COMMAND_SEARCHCTRL_SEARCH_BTN, wxCommandEventHandler( MediaPlayerSettingsBase::OnSearch ), NULL, this );
	treeOptions->Connect( wxEVT_COMMAND_TREE_SEL_CHANGED, wxTreeEventHandler( MediaPlayerSettingsBase::OnTreeSelected ), NULL, this );
	treeOptions->Connect( wxEVT_COMMAND_TREE_SEL_CHANGING, wxTreeEventHandler( MediaPlayerSettingsBase::OnTreeChanging ), NULL, this );
	sizerButtonsCancel->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( MediaPlayerSettingsBase::OnCancel ), NULL, this );
	sizerButtonsOK->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( MediaPlayerSettingsBase::OnOk ), NULL, this );
}

MediaPlayerSettingsBase::~MediaPlayerSettingsBase()
{
	// Disconnect Events
	searchOptions->Disconnect( wxEVT_COMMAND_SEARCHCTRL_SEARCH_BTN, wxCommandEventHandler( MediaPlayerSettingsBase::OnSearch ), NULL, this );
	treeOptions->Disconnect( wxEVT_COMMAND_TREE_SEL_CHANGED, wxTreeEventHandler( MediaPlayerSettingsBase::OnTreeSelected ), NULL, this );
	treeOptions->Disconnect( wxEVT_COMMAND_TREE_SEL_CHANGING, wxTreeEventHandler( MediaPlayerSettingsBase::OnTreeChanging ), NULL, this );
	sizerButtonsCancel->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( MediaPlayerSettingsBase::OnCancel ), NULL, this );
	sizerButtonsOK->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( MediaPlayerSettingsBase::OnOk ), NULL, this );

}
