// Local headers
#include "vaccineFinderApp.h"
#include "mainFrame.h"

IMPLEMENT_APP(VaccineFinderApp);


const wxString VaccineFinderApp::title = _T("Vaccine Finder");
const wxString VaccineFinderApp::internalName = _T("VaccineFinder");
const wxString VaccineFinderApp::creator = _T("KRL");

bool VaccineFinderApp::OnInit()
{
	SetAppName(internalName);
	SetVendorName(creator);

	mainFrame = new MainFrame();

	if (!mainFrame)
		return false;

	mainFrame->Show(true);

	return true;
}
