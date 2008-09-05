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

#ifndef __JackTime_APPLE__
#define __JackTime_APPLE__

#ifdef __cplusplus
extern "C"
{
#endif

#include <mach/mach_time.h>
#include <unistd.h>

    extern EXPORT double __jack_time_ratio;

    static inline jack_time_t GetMicroSeconds(void) 
    {
        return (jack_time_t) (mach_absolute_time () * __jack_time_ratio);
    }

    /* This should only be called ONCE per process. */
    void InitTime();

    static inline void JackSleep(int usec) 
    {
        usleep(usec);
    }

#ifdef __cplusplus
}
#endif


#endif
