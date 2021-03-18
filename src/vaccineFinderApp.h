
#ifndef VACCINE_FINDER_APP_H_
#define VACCINE_FINDER_APP_H_

// wxWidgets headers
#include <wx/wx.h>

// Local forward declarations
class MainFrame;

class VaccineFinderApp : public wxApp
{
public:
	// Initialization function
	bool OnInit();

	// The name of the application
	static const wxString title;// As displayed
	static const wxString internalName;// Internal
	static const wxString creator;

private:
	MainFrame *mainFrame = nullptr;
};

// Declare the application object (have wxWidgets create the wxGetApp() function)
DECLARE_APP(VaccineFinderApp);

#endif// VACCINE_FINDER_APP_H_
