// File:  jeffersonTarget.h
// Date:  3/3/2021
// Auth:  K. Loux
// Desc:  Routine for checking for availability at Rite Aid pharmacies.

#ifndef RITE_AID_TARGET_H_
#define RITE_AID_TARGET_H_

// Local headers
#include "finderTarget.h"

class RiteAidTarget : public FinderTarget
{
public:
	RiteAidTarget(const UString::String& url, MainFrame* mainFrame, const std::vector<UString::String>& locations,
		const unsigned int& checkPeriod, const bool& phillyMode) : FinderTarget(url, mainFrame,
			checkPeriod, _T("Rite Aid Checker")), locations(locations), phillyMode(phillyMode) {}
	~RiteAidTarget();

protected:
	bool AppointmentsAvailable(std::string& message) override;

	State DoFoundAppointmentStateChange() const override { return State::NormalCheck; }

private:
	const std::vector<UString::String> locations;
	const bool phillyMode;

	struct RefererData : public ModificationData
	{
		std::string referer;
	};

	static bool SetOptions(CURL* curl, const ModificationData*);
	static bool SetOptionsWithReferer(CURL* curl, const ModificationData* data);

	static UString::String GetFindStoresURL(const UString::String& location);
	static UString::String GetStatusCheckURL(const unsigned int& storeNumber);

	static const std::string cookieFile;
	struct curl_slist* headerList = nullptr;

	struct Location
	{
		unsigned int storeNumber;
		UString::String address;
		UString::String city;
		UString::String state;
		UString::String zip;

		bool postponeChecking = false;
		std::chrono::system_clock::time_point postponedUntil;
	};

	std::vector<Location> cachedLocations;
	std::chrono::system_clock::time_point cacheUpdatedTime;
	bool UpdateCachedLocations();

	bool ParseLocations(const std::string& response, std::vector<Location>& data) const;
	static bool ParseStatus(const std::string& response, bool& available);
};

#endif// RITE_AID_TARGET_H_
