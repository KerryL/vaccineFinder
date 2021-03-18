// File:  jeffersonTarget.h
// Date:  3/3/2021
// Auth:  K. Loux
// Desc:  Routine for checking for availability at Jefferson Hospitals.

#ifndef JEFFERSON_TARGET_H_
#define JEFFERSON_TARGET_H_

// Local headers
#include "finderTarget.h"

class JeffersonTarget : public FinderTarget
{
public:
	JeffersonTarget(const UString::String& url, MainFrame* mainFrame,
		const unsigned int& checkPerod) : FinderTarget(url, mainFrame, checkPerod, _T("Jefferson")) {}

protected:
	bool AppointmentsAvailable(std::string& message) override;

private:
	static bool SetOptions(CURL* curl, const ModificationData*);
	static bool DoesNotHaveThreeRegistrationFullStatements(const std::string& html);
};

#endif// JEFFERSON_TARGET_H_
