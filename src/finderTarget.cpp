// File:  finderTarget.cpp
// Date:  3/3/2021
// Auth:  K. Loux
// Desc:  Web site target to check.

// Local headers
#include "finderTarget.h"
#include "mainFrame.h"

// wxWidgets headers
#include <wx/wx.h>

const UString::String FinderTarget::userAgent(_T("vaccineFinder"));

FinderTarget::FinderTarget(const UString::String& url, MainFrame* mainFrame,
	const unsigned int& checkPeriodSeconds, const UString::String& name) : JSONInterface(userAgent), url(url), name(name),
	checkPeriod(std::chrono::seconds(checkPeriodSeconds)), mainFrame(mainFrame)
{
	BeginCheckLoop();
}

FinderTarget::~FinderTarget()
{
	Stop();
	if (checkThread.joinable())
		checkThread.join();
}

void FinderTarget::BeginCheckLoop()
{
	checkThread = std::thread(&FinderTarget::CheckThreadEntry, this);
}

void FinderTarget::SendLogMessage(const std::string& s) const
{
	wxTheApp->GetTopWindow()->GetEventHandler()->CallAfter(std::bind(&MainFrame::SendMessageForHistory, mainFrame, s));
}

bool FinderTarget::OnAppointmentsAvailable(const std::string& appointmentInfo)
{
	wxTheApp->GetTopWindow()->GetEventHandler()->CallAfter(std::bind(&MainFrame::SendMessageForHistory, mainFrame, UString::ToNarrowString(url) + "\n" + appointmentInfo));
	wxTheApp->GetTopWindow()->GetEventHandler()->CallAfter(std::bind(&MainFrame::DoAppointmentNotification, mainFrame, UString::ToNarrowString(url) + "\n" + appointmentInfo));

	return true;
}

UString::String FinderTarget::ReplaceAll(const UString::String&s, const UString::String& match, const UString::String& replaceWith)
{
	UString::String out(s);
	UString::String::size_type index(0);
	while (index = out.find(match, index), index != UString::String::npos)
		out.replace(index, match.length(), replaceWith);

	return out;
}

void FinderTarget::CheckThreadEntry()
{
	SendLogMessage("Beginning " + UString::ToNarrowString(name) + " search...");
	auto fastCheckEndTime(std::chrono::system_clock::now());
	while (!stop)
	{
		state = State::NormalCheck;

		std::string message;
		if (AppointmentsAvailable(message))
		{
			SendLogMessage("Found appointment!");
			OnAppointmentsAvailable(message);
			state = DoFoundAppointmentStateChange();
		}

		Sleep();
	}
}

void FinderTarget::Sleep()
{
	std::unique_lock<std::mutex> lock(mutex);
	if (state == State::NormalCheck)
		stopCondition.wait_for(lock, checkPeriod);
	else if (state == State::FoundAppointmentDelay)
		stopCondition.wait_for(lock, checkPeriod * 4);
}

void FinderTarget::Stop()
{
	std::lock_guard<std::mutex> lock(mutex);
	stop = true;
	stopCondition.notify_all();
}
