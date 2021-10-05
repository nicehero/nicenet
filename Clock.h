#ifndef __NICE_CLOCK___
#define __NICE_CLOCK___
#include "Module.h"
#include <chrono>
namespace nicehero
{
	struct timeval
	{
		time_t  tv_sec;         /* seconds */
		long    tv_usec;        /* and microseconds */
	};
	class Clock
		:public Module<Clock>
	{
	public:
		Clock();
		ui64 getTime();
		ui64 getTimeMS();

		ui64 getSeconds();
		ui64 getMilliSeconds();

		timeval	m_current;	
		ui32 m_second;			
		ui64 m_millisecond;	
		ui64 m_currentMS;	

		double m_scale;			
		ui32 m_retSecond;		
		ui64 m_retMillisecond;
		ui32 m_tailMillisecond;

	};


}

#define nNow nicehero::Clock::getInstance()->getTime()
#endif
