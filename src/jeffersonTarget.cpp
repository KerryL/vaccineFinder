// File:  jeffersonTarget.cpp
// Date:  3/3/2021
// Auth:  K. Loux
// Desc:  Routine for checking for availability at Jefferson Hospitals.

// Local headers
#include "jeffersonTarget.h"
#include "email/curlUtilities.h"

bool JeffersonTarget::AppointmentsAvailable(std::string&)
{
	std::string response;
	if (!DoCURLGet(url, response))
	{
		SendLogMessage("Jefferson check failed");
		return false;
	}

	return DoesNotHaveThreeRegistrationFullStatements(response);
}

bool JeffersonTarget::DoesNotHaveThreeRegistrationFullStatements(const std::string& html)
{
	std::string::size_type position(0);
	unsigned int count(0);
	const std::string testString("Registration is currently full at this location.");
	do
	{
		position = html.find(testString, position + 1);
		if (position != std::string::npos)
			++count;
	} while (position != std::string::npos);

	return count < 3;
}

bool JeffersonTarget::SetOptions(CURL* curl, const ModificationData*)
{
	// This is required for multi-threaded applications
	if (CURLUtilities::CURLCallHasError(curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L), _T("Failed to disable signaling")))
		return false;

	return true;
}
