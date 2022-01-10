#include "Clock.h"
#include <time.h>
#ifdef WIN32
#include <Windows.h>
#else
#include<sys/time.h>
#endif

MODULE_IMPL(nicehero::Clock)

namespace nicehero
{

#ifdef WIN32
	typedef long long LONGLONG;
#define FACTOR (0x19db1ded53e8000LL)

	static inline i64
		systime()
	{
		LARGE_INTEGER x;
		FILETIME ft;
		GetSystemTimeAsFileTime(&ft);
		x.HighPart = ft.dwHighDateTime;
		x.LowPart = ft.dwLowDateTime;
		x.QuadPart -= FACTOR;		/* Add conversion factor for UNIX vs. Windows base time */
		x.QuadPart /= 10;		/* Convert to microseconds */
		return x.QuadPart;
	}

	int gettimeofday(struct ntimeval *tv, struct timezone *tz)
	{
		LONGLONG now = systime();

		tv->tv_sec = static_cast<time_t>(now / 1000000);
		tv->tv_usec = static_cast<long>(now % 1000000);
		return 0;
	}
#endif


	Clock::Clock()
	{
		m_scale = 1.0;
#ifdef WIN32
		m_millisecond = m_tailMillisecond = timeGetTime();
#endif

	}

	ui64 Clock::getTime()
	{
		return time(0);
	}

	ui64 Clock::getTimeMS()
	{
		return time(0) * 1000;
	}

	ui64 Clock::getSeconds()
	{
#ifdef WIN32
		gettimeofday(&m_current, NULL);
#else
		::timeval current;
		gettimeofday(&current, NULL);
		m_current.tv_sec = current.tv_sec;
		m_current.tv_usec = current.tv_usec;
#endif

#ifdef WIN32
		ui32 now = timeGetTime();
		ui32 up = now - m_tailMillisecond;
		m_tailMillisecond = now;
		m_millisecond += up;
		m_second = static_cast<ui32>(m_millisecond / 1000);
#else
		timespec ts;
		clock_gettime(CLOCK_MONOTONIC, &ts);
		m_second = ts.tv_sec;
		m_millisecond = (i64)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
#endif
		m_currentMS = m_current.tv_sec * 1000;
		m_currentMS += m_current.tv_usec / 1000;
		m_retMillisecond = m_millisecond;// *m_scale;
		m_retSecond = static_cast<ui32>(m_retMillisecond / 1000);
		return m_retSecond;
	}

	ui64 Clock::getMilliSeconds()
	{
#ifdef WIN32
		gettimeofday(&m_current, NULL);
#else
		::timeval current;
		gettimeofday(&current, NULL);
		m_current.tv_sec = current.tv_sec;
		m_current.tv_usec = current.tv_usec;
#endif

#ifdef WIN32
		ui32 now = timeGetTime();
		ui32 up = now - m_tailMillisecond;
		m_tailMillisecond = now;
		m_millisecond += up;
		m_second = static_cast<ui32>(m_millisecond / 1000);
#else
		timespec ts;
		clock_gettime(CLOCK_MONOTONIC, &ts);
		m_second = ts.tv_sec;
		m_millisecond = (i64)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
#endif
		m_currentMS = m_current.tv_sec * 1000;
		m_currentMS += m_current.tv_usec / 1000;
		m_retMillisecond = m_millisecond;// *m_scale;
		m_retSecond = static_cast<ui32>(m_retMillisecond / 1000);
		return m_retMillisecond;
	}

}
