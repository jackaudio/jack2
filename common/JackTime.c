/*
	Copyright (C) 2001-2003 Paul Davis
	Copyright (C) 2004-2008 Grame

	This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

*/

#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#include "JackTime.h"
#include "JackError.h"

#ifdef __APPLE__

double __jack_time_ratio;

/* This should only be called ONCE per process. */
void InitTime()
{
	jack_log("InitTime");
	mach_timebase_info_data_t info;
    mach_timebase_info(&info);
    __jack_time_ratio = ((float)info.numer / info.denom) / 1000;
}

#endif

#ifdef WIN32

EXPORT LARGE_INTEGER _jack_freq;

void InitTime()
{
	QueryPerformanceFrequency(&_jack_freq);
	jack_log("InitTime freq = %ld  %ld", _jack_freq.HighPart, _jack_freq.LowPart);
	_jack_freq.QuadPart = _jack_freq.QuadPart / 1000000; // by usec
}

jack_time_t GetMicroSeconds(void) 
{
	LARGE_INTEGER t1;
	QueryPerformanceCounter (&t1);
	return (jack_time_t)(((double)t1.QuadPart)/((double)_jack_freq.QuadPart));		
}

// TODO
#endif

#ifdef linux

#ifdef GETCYCLE_TIME

#include <stdio.h>
jack_time_t GetMhz(void)
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
			jack_error ("FATAL: cannot locate cpu MHz in "
				    "/proc/cpuinfo\n");
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

jack_time_t __jack_cpu_mhz;

void InitTime()
{
	__jack_cpu_mhz = GetMhz();
}

#else
void InitTime()
{}

#endif 

#endif 
