// File:  cvsTarget.cpp
// Date:  3/3/2021
// Auth:  K. Loux
// Desc:  Routine for checking for availability at CVS pharmacies.

// Local headers
#include "cvsTarget.h"
#include "email/curlUtilities.h"
#include "mainFrame.h"

// wxWidgets headers
#include <wx/wx.h>

// Standard C++ headers
#include <algorithm>

const std::string CVSTarget::cookieFile(".cvsCookies");

CVSTarget::~CVSTarget()
{
	if (headerList)
		curl_slist_free_all(headerList);
}

bool CVSTarget::AppointmentsAvailable(std::string& message)
{
	std::string response;
	if (!DoCURLGet(url, response, &SetOptions))
	{
		SendLogMessage("CVS base get failed");
		return false;
	}

	RefererData d;
	d.referer = UString::ToNarrowString(url);
	if (!DoCURLGet(_T("https://www.cvs.com/immunizations/covid-19-vaccine.vaccine-status.PA.json?vaccineinfo"), response, &SetOptionsWithReferer, &d))
	{
		SendLogMessage("CVS get status failed");
		return false;
	}

	bool available;
	if (!ParseResponse(response, available, message))
		return false;

	return available;
}

bool CVSTarget::SetOptions(CURL* curl, const ModificationData*)
{
	// This first one is required for multi-threaded applications
	if (CURLUtilities::CURLCallHasError(curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L), _T("Failed to disable signaling")))
		return false;

	if (CURLUtilities::CURLCallHasError(curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L), _T("Failed to enable location following")))
		return false;

	if (CURLUtilities::CURLCallHasError(curl_easy_setopt(curl, CURLOPT_COOKIEFILE, cookieFile.c_str()), _T("Failed to load the cookie file")))
		return false;

	if (CURLUtilities::CURLCallHasError(curl_easy_setopt(curl, CURLOPT_COOKIEJAR, cookieFile.c_str()), _T("Failed to enable saving cookies")))
		return false;

	return true;
}

bool CVSTarget::SetOptionsWithReferer(CURL* curl, const ModificationData* data)
{
	SetOptions(curl, nullptr);
	auto referer(reinterpret_cast<const RefererData*>(data));
	if (CURLUtilities::CURLCallHasError(curl_easy_setopt(curl, CURLOPT_REFERER, referer->referer.c_str()), _T("Failed to set referer")))
		return false;

	return true;
}

bool CVSTarget::ParseResponse(const std::string& response, bool& appointmentsAvailable, std::string& message) const
{
	appointmentsAvailable = false;
	cJSON* root(cJSON_Parse(response.c_str()));
	if (!root)
	{
		Cerr << "Failed to parse root node\n";
		return false;
	}

	cJSON* payload(cJSON_GetObjectItem(root, "responsePayloadData"));
	if (!payload)
	{
		cJSON_free(root);
		Cerr << "Failed to find payload node\n";
		return false;
	}

	bool bookingComplete;
	if (!ReadJSON(payload, _T("isBookingCompleted"), bookingComplete))
	{
		cJSON_free(root);
		Cerr << "Failed to read isBookingCompleted\n";
		return false;
	}

	if (bookingComplete)
	{
		cJSON_free(root);
		return true;
	}

	// To avoid missing potential locations, we'll exclude locations that we know are too far away, but include unknown locations
	cJSON* data(cJSON_GetObjectItem(payload, "data"));
	if (!data)
	{
		cJSON_free(root);
		Cerr << "Failed to find data node\n";
		return false;
	}

	cJSON* paLocations(cJSON_GetObjectItem(data, "PA"));
	if (!paLocations)
	{
		cJSON_free(root);
		Cerr << "Failed to find PA locations array\n";
		return false;
	}

	std::vector<std::string> locations;
	for (int i = 0; i < cJSON_GetArraySize(paLocations); ++i)
	{
		auto location(cJSON_GetArrayItem(paLocations, i));
		if (!location)
		{
			Cerr << "Failed to get location object\n";
			continue;
		}

		UString::String status;
		if (!ReadJSON(location, _T("status"), status))
		{
			Cerr << "Failed to get status string\n";
			continue;
		}

		if (status == _T("Fully Booked"))
			continue;

		// If it wasn't fully booked, appointments are available; see if we want to exclude the location
		UString::String city;
		if (!ReadJSON(location, _T("city"), city))
		{
			Cerr << "Failed to get city string\n";
			continue;
		}

		locations.push_back(UString::ToNarrowString(city));
		if (std::find(excludeLocations.begin(), excludeLocations.end(), city) != excludeLocations.end())
			continue;

		appointmentsAvailable = true;
		message += UString::ToNarrowString(city) + '\n';
	}

	wxTheApp->GetTopWindow()->GetEventHandler()->CallAfter(std::bind(&MainFrame::UpdateCVSLocations, mainFrame, locations));

	cJSON_free(root);
	return true;
}

std::vector<UString::String> CVSTarget::MakeListAllCaps(const std::vector<UString::String>& list)
{
	std::vector<UString::String> ucList(list);
	for (auto& s : ucList)
		std::transform(s.begin(), s.end(), s.begin(), ::towupper);// or ::toupper if std::string
	return ucList;
}
