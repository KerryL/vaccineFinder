// File:  finderTarget.h
// Date:  3/3/2021
// Auth:  K. Loux
// Desc:  Web site target to check.

#ifndef FINDER_TARGET_H_
#define FINDER_TARGET_H_

// Local headers
#include "utilities/uString.h"
#include "email/jsonInterface.h"
#include "email/emailSender.h"

// Standard C++ headers
#include <thread>
#include <atomic>
#include <condition_variable>
#include <mutex>

// for cURL
typedef void CURL;

// Local forward declarations
class MainFrame;

class FinderTarget : public JSONInterface
{
public:
	FinderTarget(const UString::String& url, MainFrame* mainFrame, const unsigned int& checkPeriodSeconds, const UString::String& name);
	virtual ~FinderTarget();

	void BeginCheckLoop();
	void Stop();

protected:
	const UString::String url;
	const UString::String name;
	MainFrame* mainFrame;

	void SendLogMessage(const std::string& s) const;

	std::atomic<bool> stop = false;
	std::condition_variable stopCondition;
	std::mutex mutex;

	virtual bool AppointmentsAvailable(std::string& message) = 0;

	enum class State
	{
		NormalCheck,
		FoundAppointmentDelay
	};

	State state = State::NormalCheck;

	virtual State DoFoundAppointmentStateChange() const { return State::FoundAppointmentDelay; }

	const std::chrono::system_clock::duration checkPeriod;

private:
	static const UString::String userAgent;

	bool OnAppointmentsAvailable(const std::string& appointmentInfo);

	void Sleep();

	std::thread checkThread;
	void CheckThreadEntry();

	UString::String ReplaceAll(const UString::String&s, const UString::String& match, const UString::String& replaceWith);
};

#endif// FINDER_TARGET_H_
