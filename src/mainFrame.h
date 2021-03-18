#ifndef MAIN_FRAME_H_
#define MAIN_FRAME_H_

// Local headers
#include"finderTarget.h"

// wxWidgets headers
#include <wx/wx.h>

// Standard C++ headers
#include <vector>
#include <memory>

// The main frame class
class MainFrame : public wxFrame
{
public:
	MainFrame();
	~MainFrame();

	void UpdateCVSLocations(const std::vector<std::string> locations);
	void SendMessageForHistory(const std::string s);
	void DoAppointmentNotification(const std::string message);

private:
	static const wxString configFileName;

	// Functions that do some of the frame initialization and control positioning
	void CreateControls();
	void SetProperties();

	wxSizer* CreateRiteAidControls(wxWindow* parent);
	wxSizer* CreateCVSControls(wxWindow* parent);
	wxSizer* CreateHistoryControls(wxWindow* parent);

	wxCheckListBox* cvsLocationCheckListBox;
	wxTextCtrl* riteAidLocationTextCtrl;
	wxTextCtrl* historyTextCtrl;
	wxRadioButton* nonPhillyRadioButtion;
	wxRadioButton* phillyRadioButtion;

	// The event IDs
	enum MainFrameEventID
	{
		idUpdateButton = wxID_HIGHEST + 200
	};

	// Button events
	void UpdateButtonClickedEvent(wxCommandEvent& event);

	void WriteConfiguration();
	void LoadConfiguration();

	std::vector<std::unique_ptr<FinderTarget>> finderTargets;
	wxArrayString GetRiteAidLocations(const bool& encoded) const;
	wxArrayString GetCVSExcludeLocations() const;

	static std::string GetTimeStamp();
	static wxString ArrayToConfigString(const wxArrayString& a);
	static wxArrayString ConfigStringToArray(const wxString& s);
	static std::vector<UString::String> ToUStringVector(const wxArrayString& a);

	DECLARE_EVENT_TABLE();
};

#endif// MAIN_FRAME_H_
