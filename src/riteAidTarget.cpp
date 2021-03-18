// File:  riteAidTarget.cpp
// Date:  3/3/2021
// Auth:  K. Loux
// Desc:  Routine for checking for availability at Rite Aid pharmacies.

// Local headers
#include "riteAidTarget.h"
#include "email/curlUtilities.h"

const std::string RiteAidTarget::cookieFile(".riteAidCookies");

RiteAidTarget::~RiteAidTarget()
{
	if (headerList)
		curl_slist_free_all(headerList);
}

bool RiteAidTarget::AppointmentsAvailable(std::string& message)
{
	// Get base page (always do this to keep cookies current)
	std::string response;
	if (!DoCURLGet(url, response, SetOptions))
	{
		SendLogMessage("Rite Aid get failed");
		return false;
	}

	const auto now(std::chrono::system_clock::now());
	if (cachedLocations.empty() || cacheUpdatedTime + std::chrono::hours(24) > now)
	{
		if (!UpdateCachedLocations())
			return false;
	}

	// Find stores - only returns 10 nearest locations, so need to check multiple locations to be thorough
	bool available(false);
	std::ostringstream messageSS;
	for (auto& store : cachedLocations)
	{
		if (store.postponeChecking)
		{
			if (now > store.postponedUntil)
				store.postponeChecking = false;
			else
				continue;
		}

		RefererData d;
		d.referer = UString::ToNarrowString(url);
		
		// Check availability
		// This goes fast enough that it's not worth trying to notify users faster - check all locations then send one notification
		if (!DoCURLGet(GetStatusCheckURL(store.storeNumber), response, SetOptionsWithReferer, &d))
		{
			SendLogMessage("Rite Aid check status failed");
			return false;
		}

		bool locationHasAvailability;
		if (!ParseStatus(response, locationHasAvailability))
			continue;

		if (locationHasAvailability)
		{
			store.postponeChecking = true;
			store.postponedUntil = now + checkPeriod * 10;
			available = true;

			messageSS << "Rite Aid Location Info:  " << UString::ToNarrowString(store.address) << ", "
				<< UString::ToNarrowString(store.city) << ", " << UString::ToNarrowString(store.state) << " " << UString::ToNarrowString(store.zip) << '\n';
		}
	}

	message = messageSS.str();
	return available;
}

bool RiteAidTarget::UpdateCachedLocations()
{
	cachedLocations.clear();
	for (const auto& loc : locations)// Go through user-specified locations to check
	{
		RefererData d;
		std::string response;
		d.referer = UString::ToNarrowString(url);
		if (!DoCURLGet(GetFindStoresURL(loc), response, SetOptionsWithReferer, &d))
		{
			SendLogMessage("Rite Aid get stores failed");
			return false;
		}

		std::vector<Location> data;
		if (!ParseLocations(response, data))
			continue;

		for (const auto& newLocation : data)
		{
			bool alreadyCached(false);
			for (const auto& c : cachedLocations)
			{
				if (c.storeNumber == newLocation.storeNumber)
				{
					alreadyCached = true;
					break;
				}
			}

			if (!alreadyCached)
				cachedLocations.push_back(newLocation);
		}
	}

	return true;
}

UString::String RiteAidTarget::GetFindStoresURL(const UString::String& location)
{
	return _T("https://www.riteaid.com/services/ext/v2/stores/getStores?address=") + location + _T("&attrFilter=PREF-112&fetchMechanismVersion=2&radius=50");
}

UString::String RiteAidTarget::GetStatusCheckURL(const unsigned int& storeNumber)
{
	UString::OStringStream ss;
	ss << "https://www.riteaid.com/services/ext/v2/vaccine/checkSlots?storeNumber=" << storeNumber;
	return ss.str();
}

bool RiteAidTarget::SetOptions(CURL* curl, const ModificationData*)
{
	// This is required for multi-threaded applications
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

bool RiteAidTarget::SetOptionsWithReferer(CURL* curl, const ModificationData* data)
{
	SetOptions(curl, nullptr);
	auto referer(reinterpret_cast<const RefererData*>(data));
	if (CURLUtilities::CURLCallHasError(curl_easy_setopt(curl, CURLOPT_REFERER, referer->referer.c_str()), _T("Failed to set referer")))
		return false;

	return true;
}

bool RiteAidTarget::ParseLocations(const std::string& response, std::vector<Location>& data) const
{
	cJSON* root(cJSON_Parse(response.c_str()));
	if (!root)
	{
		Cerr << "Failed to parse root node\n";
		return false;
	}

	cJSON* dataObject(cJSON_GetObjectItem(root, "Data"));
	if (!dataObject)
	{
		Cerr << "Failed to get data object\n";
		cJSON_Delete(root);
		return false;
	}

	cJSON* storeArray(cJSON_GetObjectItem(dataObject, "stores"));
	if (!storeArray)
	{
		Cerr << "Failed to get stores array\n";
		cJSON_Delete(root);
		return false;
	}

	for (int i = 0; i < cJSON_GetArraySize(storeArray); ++i)
	{
		cJSON* item(cJSON_GetArrayItem(storeArray, i));
		if (!item)
		{
			Cerr << "Failed to get stores object\n";
			cJSON_Delete(root);
			return false;
		}

		Location loc;
		if (!ReadJSON(item, _T("storeNumber"), loc.storeNumber))
		{
			Cerr << "Failed to read store number\n";
			cJSON_Delete(root);
			return false;
		}

		if (!ReadJSON(item, _T("address"), loc.address))
		{
			Cerr << "Failed to read address\n";
			cJSON_Delete(root);
			return false;
		}

		if (!ReadJSON(item, _T("city"), loc.city))
		{
			Cerr << "Failed to read city\n";
			cJSON_Delete(root);
			return false;
		}

		if (!ReadJSON(item, _T("state"), loc.state))
		{
			Cerr << "Failed to read state\n";
			cJSON_Delete(root);
			return false;
		}

		if (!ReadJSON(item, _T("zipcode"), loc.zip))
		{
			Cerr << "Failed to read zip code\n";
			cJSON_Delete(root);
			return false;
		}

		if (loc.state != UString::String(_T("PA")))
			return true;

		if ((phillyMode && loc.city == UString::String(_T("Philadelphia"))) ||
			(!phillyMode && loc.city != UString::String(_T("Philadelphia"))))
			data.push_back(loc);
	}

	return true;
}

bool RiteAidTarget::ParseStatus(const std::string& response, bool& available)
{
	cJSON* root(cJSON_Parse(response.c_str()));
	if (!root)
	{
		Cerr << "Failed to parse root node\n";
		return false;
	}

	cJSON* dataObject(cJSON_GetObjectItem(root, "Data"));
	if (!dataObject)
	{
		Cerr << "Failed to get data object\n";
		cJSON_Delete(root);
		return false;
	}

	cJSON* slots(cJSON_GetObjectItem(dataObject, "slots"));
	if (!slots)
	{
		Cerr << "Failed to get slots object\n";
		cJSON_Delete(root);
		return false;
	}

	// Not sure what the difference is between one and two?  First/second dose availability?
	bool one, two;
	if (!ReadJSON(slots, _T("1"), one))
	{
		Cerr << "Failed to read one\n";
		cJSON_Delete(root);
		return false;
	}

	if (!ReadJSON(slots, _T("2"), two))
	{
		Cerr << "Failed to read two\n";
		cJSON_Delete(root);
		return false;
	}

	available = one || two;
	/*if (available)// Until we understand one/two, provide additional diagnostics
	// I still don't understand 100%, but I have not yet seen a response different from 1:true,2:false, so we'll ignore this for now 3/6/2021
		Cout << "one:  " << static_cast<int>(one) << "; two:  " << static_cast<int>(two) << std::endl;*/

	return true;
}
