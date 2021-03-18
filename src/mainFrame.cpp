
// Local headers
#include"mainFrame.h"
#include "riteAidTarget.h"
#include "cvsTarget.h"
#include "jeffersonTarget.h"
#include "vaccineFinderApp.h"

// wxWidgets headers
#include <wx/fileconf.h>

// Standard C++ headers
#include <iomanip>

const wxString MainFrame::configFileName(_T("vaccineFinder.config"));

MainFrame::MainFrame() : wxFrame(nullptr, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxDEFAULT_FRAME_STYLE)
{
	curl_global_init(CURL_GLOBAL_ALL);// Do this before launching threads

	CreateControls();
	SetProperties();
}

MainFrame::~MainFrame()
{
	WriteConfiguration();
	curl_global_cleanup();
}

BEGIN_EVENT_TABLE(MainFrame, wxFrame)
	EVT_BUTTON(idUpdateButton, MainFrame::UpdateButtonClickedEvent)
END_EVENT_TABLE();

void MainFrame::CreateControls()
{
	wxSizer* topSizer(new wxBoxSizer(wxHORIZONTAL));
	wxPanel* panel(new wxPanel(this));
	topSizer->Add(panel, 1, wxGROW);

	wxSizer* panelSizer(new wxBoxSizer(wxHORIZONTAL));
	panel->SetSizer(panelSizer);

	wxSizer* mainSizer(new wxBoxSizer(wxVERTICAL));
	panelSizer->Add(mainSizer, wxSizerFlags().Expand().Border(wxALL, 5).Proportion(1));

	nonPhillyRadioButtion = new wxRadioButton(panel, wxID_ANY, _T("Search outside of Philadelphia"));
	phillyRadioButtion = new wxRadioButton(panel, wxID_ANY, _T("Search within Philadelphia"));
	mainSizer->Add(nonPhillyRadioButtion);
	mainSizer->Add(phillyRadioButtion);

	wxSizer* inputSizer(new wxBoxSizer(wxHORIZONTAL));
	mainSizer->Add(inputSizer, wxSizerFlags().Expand().Proportion(1));

	inputSizer->Add(CreateRiteAidControls(panel), wxSizerFlags().Expand().Proportion(1));
	inputSizer->Add(CreateCVSControls(panel), wxSizerFlags().Expand().Proportion(1));
	mainSizer->Add(CreateHistoryControls(panel), wxSizerFlags().Expand());
	mainSizer->Add(new wxButton(panel, idUpdateButton, _T("Begin/Update Search")));

	SetSizerAndFit(topSizer);
}

wxSizer* MainFrame::CreateRiteAidControls(wxWindow* parent)
{
	wxStaticBoxSizer* s(new wxStaticBoxSizer(wxVERTICAL, parent, _T("Rite Aid")));
	s->Add(new wxStaticText(s->GetStaticBox(), wxID_ANY, _T("Search areas (zip code or city, state):")));
	riteAidLocationTextCtrl = new wxTextCtrl(s->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE);
	s->Add(riteAidLocationTextCtrl, wxSizerFlags().Expand().Proportion(1));
	return s;
}

wxSizer* MainFrame::CreateCVSControls(wxWindow* parent)
{
	wxStaticBoxSizer* s(new wxStaticBoxSizer(wxVERTICAL, parent, _T("CVS")));
	s->Add(new wxStaticText(s->GetStaticBox(), wxID_ANY, _T("Select cities to search:")));
	cvsLocationCheckListBox = new wxCheckListBox(s->GetStaticBox(), wxID_ANY);
	s->Add(cvsLocationCheckListBox, wxSizerFlags().Expand().Proportion(1));
	return s;
}

wxSizer* MainFrame::CreateHistoryControls(wxWindow* parent)
{
	wxSizer* s(new wxBoxSizer(wxVERTICAL));
	historyTextCtrl = new wxTextCtrl(parent, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE);
	historyTextCtrl->Enable(false);
	historyTextCtrl->SetMinSize(wxSize(400, 200));
	s->Add(historyTextCtrl, wxSizerFlags().Expand().Proportion(1));
	return s;
}

void MainFrame::SetProperties()
{
	SetTitle(VaccineFinderApp::title);
	SetName(VaccineFinderApp::internalName);
	Center();
	LoadConfiguration();
}

void MainFrame::UpdateCVSLocations(const std::vector<std::string> locations)
{
	wxArrayString wxLocations;
	for (const auto& loc : locations)
		wxLocations.Add(loc);

	if (wxLocations == cvsLocationCheckListBox->GetStrings())
		return;

	const auto excludeLocations(GetCVSExcludeLocations());
	cvsLocationCheckListBox->Clear();
	cvsLocationCheckListBox->InsertItems(wxLocations, 0);

	for (unsigned int i = 0; i < cvsLocationCheckListBox->GetCount(); ++i)
		cvsLocationCheckListBox->Check(i);

	for (const auto& e : excludeLocations)
	{
		for (unsigned int i = 0; i < cvsLocationCheckListBox->GetCount(); ++i)
		{
			if (cvsLocationCheckListBox->GetString(i) == e)
			{
				cvsLocationCheckListBox->Check(i, false);
				break;
			}
		}
	}
}

void MainFrame::SendMessageForHistory(const std::string s)
{
	historyTextCtrl->AppendText(GetTimeStamp() + _T(" : ") + s + _T("\n"));
}

std::string MainFrame::GetTimeStamp()
{
	const time_t now(time(nullptr));
	const struct tm* timeInfo(localtime(&now));

	std::ostringstream timeStamp;
	timeStamp.fill('0');
	timeStamp << timeInfo->tm_year + 1900 << "-"
		<< std::setw(2) << timeInfo->tm_mon + 1 << "-"
		<< std::setw(2) << timeInfo->tm_mday << " "
		<< std::setw(2) << timeInfo->tm_hour << ":"
		<< std::setw(2) << timeInfo->tm_min << ":"
		<< std::setw(2) << timeInfo->tm_sec;

	return timeStamp.str();
}

void MainFrame::DoAppointmentNotification(const std::string message)
{
	wxMessageBox(message, _T("Found Appointment"));
}

void MainFrame::UpdateButtonClickedEvent(wxCommandEvent& WXUNUSED(event))
{
	const unsigned int cvsCheckPeriod(300);// [sec]
	const unsigned int riteAidCheckPeriod(120);// [sec]
	const unsigned int jeffersonPeriod(300);// [sec]

	finderTargets.clear();
	if (nonPhillyRadioButtion->GetValue())
	{
		finderTargets.push_back(std::make_unique<CVSTarget>(_T("https://www.cvs.com/immunizations/covid-19-vaccine"), this, cvsCheckPeriod, ToUStringVector(GetCVSExcludeLocations())));
		finderTargets.push_back(std::make_unique<JeffersonTarget>(_T("https://www.jeffersonhealth.org/coronavirus-covid-19/vaccination-clinics.html"), this, jeffersonPeriod));
	}

	finderTargets.push_back(std::make_unique<RiteAidTarget>(_T("https://www.riteaid.com/pharmacy/apt-scheduler#"), this, ToUStringVector(GetRiteAidLocations(true)), riteAidCheckPeriod, phillyRadioButtion->GetValue()));
}

wxString MainFrame::ArrayToConfigString(const wxArrayString& a)
{
	wxString s;
	for (const auto& i : a)
		s.Append(i + _T(";"));
	return s;
}

wxArrayString MainFrame::ConfigStringToArray(const wxString& s)
{
	wxArrayString a;
	std::istringstream ss(s.ToStdString());
	std::string i;
	while (std::getline(ss, i, ';'))
		a.Add(i);
	return a;
}

std::vector<UString::String> MainFrame::ToUStringVector(const wxArrayString& a)
{
	std::vector<UString::String> v;
	for (const auto& s : a)
		v.push_back(UString::ToStringType(s.ToStdString()));
	return v;
}

void MainFrame::WriteConfiguration()
{
	if (!::wxDirExists(::wxGetCwd()))
	{
		wxMessageBox(_T("Working directory is inaccessible; configuration cannot be saved."),
			_T("Error"), wxOK | wxICON_WARNING, this);
		return;
	}

	std::unique_ptr<wxFileConfig> config(std::make_unique<wxFileConfig>(_T(""), _T(""),
		configFileName, _T(""), wxCONFIG_USE_RELATIVE_PATH));

	config->Write(_T("/Window/Width"), GetSize().GetWidth());
	config->Write(_T("/Window/Height"), GetSize().GetHeight());
	config->Write(_T("/Window/XPosition"), GetPosition().x);
	config->Write(_T("/Window/YPosition"), GetPosition().y);
	config->Write(_T("/Window/IsMaximized"), IsMaximized());

	config->Write(_T("/search/phillyMode"), phillyRadioButtion->GetValue());

	config->Write(_T("/search/riteAid/locations"), ArrayToConfigString(GetRiteAidLocations(false)));
	config->Write(_T("/search/cvs/knownLocations"), ArrayToConfigString(cvsLocationCheckListBox->GetStrings()));
	config->Write(_T("/search/cvs/excludeLocations"), ArrayToConfigString(GetCVSExcludeLocations()));
}

void MainFrame::LoadConfiguration()
{
	std::unique_ptr<wxFileConfig> config(std::make_unique<wxFileConfig>(_T(""), _T(""),
		configFileName, _T(""), wxCONFIG_USE_RELATIVE_PATH));

	bool tempBool;
	if (config->Read(_T("/search/phillyMode"), &tempBool, false))
	{
		nonPhillyRadioButtion->SetValue(!tempBool);
		phillyRadioButtion->SetValue(tempBool);
	}

	wxString tempString;
	if (config->Read(_T("/search/riteAid/locations"), &tempString) && !tempString.IsEmpty())
	{
		riteAidLocationTextCtrl->Clear();
		const auto& a(ConfigStringToArray(tempString));
		for (const auto& loc : a)
			riteAidLocationTextCtrl->AppendText(loc + _T("\n"));
	}

	tempString.Clear();
	if (config->Read(_T("/search/cvs/knownLocations"), &tempString && !tempString.IsEmpty()))
	{
		cvsLocationCheckListBox->Clear();
		cvsLocationCheckListBox->InsertItems(ConfigStringToArray(tempString), 0);

		tempString.Clear();
		if (config->Read(_T("/search/cvs/excludeLocations"), &tempString) && !tempString.IsEmpty())
		{
			const auto& a(ConfigStringToArray(tempString));
			for (const auto& loc : a)
			{
				for (unsigned int i = 0; i < cvsLocationCheckListBox->GetCount(); ++i)
				{
					if (cvsLocationCheckListBox->GetString(i) == loc)
					{
						cvsLocationCheckListBox->Check(i);
						break;
					}
				}
			}
		}
	}

	int x(0), y(0);
	if (config->Read(_T("/Window/XPosition"), &x) &&
		config->Read(_T("/Window/YPosition"), &y))
		SetPosition(wxPoint(x, y));

	bool isMaximized(false);
	if (config->Read(_T("/Window/IsMaximized"), &isMaximized) && isMaximized)
	{
		Maximize();
		return;// Don't set width and height
	}

	int w(0), h(0);
	if (config->Read(_T("/Window/Width"), &w) &&
		config->Read(_T("/Window/Height"), &h))
		SetSize(w, h);
}

wxArrayString MainFrame::GetRiteAidLocations(const bool& encoded) const
{
	std::istringstream ss(riteAidLocationTextCtrl->GetValue().ToStdString());
	wxArrayString locations;
	std::string line;
	while (std::getline(ss, line))
	{
		wxString wxLine(line);
		if (encoded)
			wxLine.Replace(_T(" "), _T("%20"));
		locations.Add(wxLine);
	}
	return locations;
}

wxArrayString MainFrame::GetCVSExcludeLocations() const
{
	wxArrayInt selections;
	cvsLocationCheckListBox->GetSelections(selections);
	wxArrayInt notSelections;
	for (int i = cvsLocationCheckListBox->GetCount(); i > 0; --i)
	{
		bool include(true);
		for (const auto& s : selections)
		{
			if (i == s)
			{
				include = false;
				break;
			}
		}
		if (include)
			notSelections.Add(i);
	}
	
	auto locations(cvsLocationCheckListBox->GetStrings());
	for (const auto& i : notSelections)
		locations.RemoveAt(i);
	return locations;
}
