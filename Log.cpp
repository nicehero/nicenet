#include "Log.h"
#include <stdarg.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <chrono>
#include "Clock.h"

MODULE_IMPL(nicehero::Log)
namespace nicehero
{
	const int TIMEBUF_LENGTH = 12;
	const int LOGBUF_MAXLEN = 1024 * 1024;
	char LOGBUF[LOGBUF_MAXLEN + 1] = { 0 };

	void Log::log(const char *msg, ...)
	{
		const int reservedLen = 1;//for \n
		ui64 now = Clock::getInstance()->getTime();
#ifdef WIN32
		tm tm_now;
		localtime_s(&tm_now, (time_t*)&now);
		strftime(m_timebuf, sizeof(m_timebuf), "[%H:%M:%S]: ", &tm_now);
		strncpy_s(LOGBUF, m_timebuf, TIMEBUF_LENGTH);
#else
		tm* tm_ = localtime((time_t*)&now);
		strftime(m_timebuf, sizeof(m_timebuf), "[%H:%M:%S]: ", tm_);
		strncpy(LOGBUF, m_timebuf, TIMEBUF_LENGTH);
#endif
		const int size = LOGBUF_MAXLEN - TIMEBUF_LENGTH - reservedLen;
		va_list args;
		va_start(args, msg);
		vsnprintf(LOGBUF + TIMEBUF_LENGTH, size, msg, args);
		va_end(args);
		puts(LOGBUF);
	}

	void Log::logerr(const char *msg, ...)
	{
		const int reservedLen = 1;//for \n
		ui64 now = Clock::getInstance()->getTime();
#ifdef WIN32
		tm tm_now;
		localtime_s(&tm_now, (time_t*)&now);
		strftime(m_timebuf, sizeof(m_timebuf), "[%H:%M:%S]: ", &tm_now);
		strncpy_s(LOGBUF, m_timebuf, TIMEBUF_LENGTH);
#else
		tm* tm_ = localtime((time_t*)&now);
		strftime(m_timebuf, sizeof(m_timebuf), "[%H:%M:%S]: ", tm_);
		strncpy(LOGBUF, m_timebuf, TIMEBUF_LENGTH);
#endif
		const int size = LOGBUF_MAXLEN - TIMEBUF_LENGTH - reservedLen;
		va_list args;
		va_start(args, msg);
		vsnprintf(LOGBUF + TIMEBUF_LENGTH, size, msg, args);
		va_end(args);
		puts(LOGBUF);
	}

}

