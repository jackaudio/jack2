/*
Copyright (C) 2001-2003 Paul Davis
Copyright (C) 2004-2008 Grame

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation; either version 2.1 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with this program; if not, write to the Free Software 
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

*/

#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#include "JackTime.h"
#include "JackTypes.h"
#include <unistd.h>

SERVER_EXPORT void JackSleep(long usec) 
{
	usleep(usec);
}

#ifdef GETCYCLE_TIME

	#include <stdio.h>
	#include "cycles.h"

	static jack_time_t __jack_cpu_mhz;

	static inline jack_time_t GetMhz(void)
	{
		FILE *f = fopen("/proc/cpuinfo", "r");
		if (f == 0) {
			perror("can't open /proc/cpuinfo\n");
			exit(1);
		}
	
		for (;;) {
			jack_time_t mhz;
			int ret;
			char buf[1000];
	
			if (fgets(buf, sizeof(buf), f) == NULL) {
				jack_error("FATAL: cannot locate cpu MHz in /proc/cpuinfo\n");
				exit(1);
			}
	
	#if defined(__powerpc__)
			ret = sscanf(buf, "clock\t: %" SCNu64 "MHz", &mhz);
	#elif defined( __i386__ ) || defined (__hppa__)  || defined (__ia64__) || \
	defined(__x86_64__)
			ret = sscanf(buf, "cpu MHz         : %" SCNu64, &mhz);
	#elif defined( __sparc__ )
			ret = sscanf(buf, "Cpu0Bogo        : %" SCNu64, &mhz);
	#elif defined( __mc68000__ )
			ret = sscanf(buf, "Clocking:       %" SCNu64, &mhz);
	#elif defined( __s390__  )
			ret = sscanf(buf, "bogomips per cpu: %" SCNu64, &mhz);
	#else /* MIPS, ARM, alpha */
			ret = sscanf(buf, "BogoMIPS        : %" SCNu64, &mhz);
	#endif 
			if (ret == 1) {
				fclose(f);
				return (jack_time_t)mhz;
			}
		}
	}

	SERVER_EXPORT void InitTime()
	{
		__jack_cpu_mhz = GetMhz();
	}
	
	SERVER_EXPORT jack_time_t GetMicroSeconds(void) 
	{
		return get_cycles() / __jack_cpu_mhz;
	}
#else

	#include <time.h>
	SERVER_EXPORT void InitTime()
	{}
	
	SERVER_EXPORT jack_time_t GetMicroSeconds(void) 
	{
		struct timespec ts;
		clock_gettime(CLOCK_MONOTONIC, &ts);
		return (jack_time_t)ts.tv_sec * 1000000 + ts.tv_nsec / 1000;
	}
	
#endif 

SERVER_EXPORT void SetClockSource(jack_timer_type_t source)
{}

SERVER_EXPORT const char* ClockSourceName(jack_timer_type_t source)
{
    return "";
}
