// File:  cvsTarget.h
// Date:  3/3/2021
// Auth:  K. Loux
// Desc:  Routine for checking for availability at CVS pharmacies.

#ifndef CVS_TARGET_H_
#define CVS_TARGET_H_

// Local headers
#include "finderTarget.h"

class CVSTarget : public FinderTarget
{
public:
	CVSTarget(const UString::String& url, MainFrame* mainFrame, const unsigned int& checkPeriod, const std::vector<UString::String>& excludeLocations)
		: FinderTarget(url, mainFrame, checkPeriod, _T("CVS")), excludeLocations(MakeListAllCaps(excludeLocations)) {}
	~CVSTarget();

protected:
	bool AppointmentsAvailable(std::string& message) override;

private:
	static std::vector<UString::String> MakeListAllCaps(const std::vector<UString::String>& list);
	const std::vector<UString::String> excludeLocations;

	struct RefererData : public ModificationData
	{
		std::string referer;
	};

	static bool SetOptions(CURL* curl, const ModificationData*);
	static bool SetOptionsWithReferer(CURL* curl, const ModificationData* data);

	static const std::string cookieFile;
	struct curl_slist* headerList = nullptr;

	struct LocationAvailability
	{
		std::string location;
		bool available;
	};

	bool ParseResponse(const std::string& response, bool& appointmentsAvailable, std::string& message) const;
};

#endif// CVS_TARGET_H_
